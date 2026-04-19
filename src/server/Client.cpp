#include "Client.hpp"
#include "HttpParser.hpp"
#include "HttpResponse.hpp"
#include "RequestHandler.hpp"
#include "CgiHandler.hpp"
#include "ConfigRouter.hpp"
#include "Config.hpp"
#include "Logger.hpp"
#include <sstream>

CgiProcess::CgiProcess()
    : pid(-1), pipeIn(-1), pipeOut(-1),
      bytesWritten(0), stdinDone(true),
      startTime(0), timeoutSec(DEFAULT_CGI_TIMEOUT_SECONDS), keepAlive(false) {}

void CgiProcess::reset()
{
    pid = -1;
    pipeIn = -1;
    pipeOut = -1;
    outputBuffer.clear();
    bodyToWrite.clear();
    bytesWritten = 0;
    stdinDone = true;
    startTime = 0;
    timeoutSec = DEFAULT_CGI_TIMEOUT_SECONDS;
    keepAlive = false;
}

bool CgiProcess::isActive() const
{
    return pid > 0;
}

Client::Client(int fd, Config *config, int listenFd)
    : _fd(fd), _listenFd(listenFd), _requestBuffer(""), _responseBuffer(""),
      _sendOffset(0), _config(config), _parser(new HttpParser()), _parsedOffset(0)
{
    _state = READING;
}

Client::~Client()
{
    delete _parser;
}

int Client::getFd() const
{
    return _fd;
}

int Client::getListenFd() const
{
    return _listenFd;
}

void Client::appendToRequestBuffer(const std::string &data)
{
    _requestBuffer += data;
}

void Client::setRequestBuffer(const std::string &data)
{
    _requestBuffer = data;
    _parsedOffset = 0;
}

const std::string &Client::getRequestBuffer() const
{
    return _requestBuffer;
}

void Client::clearRequestBuffer()
{
    _requestBuffer.clear();
    _parsedOffset = 0;
    if (_parser)
        _parser->reset();
}

void Client::appendToResponseBuffer(const std::string &data)
{
    _responseBuffer += data;
}

void Client::appendToResponseBuffer(const char *data, size_t len)
{
    _responseBuffer.append(data, len);
}

const std::string &Client::getResponseBuffer() const
{
    return _responseBuffer;
}

void Client::clearResponseBuffer()
{
    _responseBuffer.clear();
}

ClientState Client::getState() const
{
    return _state;
}
void Client::setState(ClientState state)
{
    _state = state;
}

size_t Client::findContentLength() const
{
    size_t headerEnd = _requestBuffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return 0;
    std::string lowerBuf = toLower(_requestBuffer.substr(0, headerEnd));
    size_t clPos = lowerBuf.find("content-length:");
    if (clPos == std::string::npos)
        return 0;
    size_t valStart = clPos + 15;
    size_t lineEnd = lowerBuf.find("\r\n", valStart);
    if (lineEnd == std::string::npos)
        lineEnd = lowerBuf.length();
    std::string clValue = trim(_requestBuffer.substr(valStart, lineEnd - valStart));
    return static_cast<size_t>(stringToInt(clValue));
}

bool Client::isRequestComplete() const
{
    size_t headerEnd = _requestBuffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return false;

    std::string lowerHeaders = toLower(_requestBuffer.substr(0, headerEnd));
    bool isChunked = lowerHeaders.find("transfer-encoding:") != std::string::npos &&
                     lowerHeaders.find("chunked") != std::string::npos;
    if (isChunked)
    {
        size_t bodyStart = headerEnd + 4;
        if (_requestBuffer.length() <= bodyStart)
            return false;
        return _requestBuffer.find("0\r\n\r\n", bodyStart) != std::string::npos;
    }

    size_t contentLength = findContentLength();
    if (contentLength > 0)
    {
        size_t bodyStart = headerEnd + 4;
        return (_requestBuffer.length() - bodyStart) >= contentLength;
    }
    return true;
}

bool Client::hasIncompleteRequest() const
{
    return _parser && _parser->getState() != HttpParser::PARSE_COMPLETE &&
           _parser->getState() != HttpParser::PARSE_ERROR;
}

size_t Client::getSendOffset() const
{
    return _sendOffset;
}

void Client::buildResponse()
{
    if (!_parser)
        return;

    if (_parsedOffset > _requestBuffer.length())
        _parsedOffset = _requestBuffer.length();

    size_t unreadBytes = _requestBuffer.length() - _parsedOffset;
    if (unreadBytes == 0)
        return;

    HttpParser::ParseResult result =
        _parser->parse(_requestBuffer.data() + _parsedOffset, unreadBytes);

    _requestBuffer = _parser->getRemainingBuffer();
    _parsedOffset = _requestBuffer.length();

    if (result == HttpParser::RESULT_ERROR)
    {
        _requestBuffer.clear();
        _parsedOffset = 0;
        buildParseErrorResponse(*_parser);
        _parser->reset();
        return;
    }

    if (result != HttpParser::RESULT_COMPLETE)
        return;

    HttpRequest &request = _parser->getRequestMutable();

    if (request.isChunked())
    {
        std::string contentType = toLower(request.getHeader("content-type"));
        if (contentType.find("multipart/form-data") != std::string::npos)
        {
            std::string boundaryTag = "boundary=";
            size_t boundaryPos = contentType.find(boundaryTag);
            if (boundaryPos == std::string::npos)
            {
                buildFinalResponse(HttpResponse::makeError(STATUS_BAD_REQUEST,
                                                           "Missing multipart boundary"),
                                   request);
                _parser->reset();
                _parsedOffset = 0;
                return;
            }

            std::string boundary = request.getHeader("content-type").substr(boundaryPos + boundaryTag.length());
            if (!boundary.empty() && boundary[boundary.length() - 1] == ';')
                boundary.erase(boundary.length() - 1);
            std::string openingMarker = "--" + boundary;
            std::string closingMarker = openingMarker + "--";
            if (request.getBody().find(openingMarker) == std::string::npos ||
                request.getBody().find(closingMarker) == std::string::npos)
            {
                buildFinalResponse(HttpResponse::makeError(STATUS_BAD_REQUEST,
                                                           "Invalid multipart body"),
                                   request);
                _parser->reset();
                _parsedOffset = 0;
                return;
            }
        }
    }

    if (!_config || _config->getServerConfigs().empty())
    {
        buildFinalResponse(HttpResponse::makeError(STATUS_INTERNAL_SERVER_ERROR,
                                                   _config ? "No server configuration" : "No configuration"),
                           request);
        return;
    }

    std::vector<ServerConfig> &servers = _config->getServerConfigs();
    size_t serverIdx = findMatchingServerIndex(servers);
    std::string hostname = extractHostname(request);
    ConfigRouter router(servers[serverIdx], hostname);
    Location location = router.route(request.getPath());

    std::string filePath = RequestHandler::resolveFilePath(request, location);
    if ((request.getMethod() == METHOD_GET || request.getMethod() == METHOD_POST ||
         request.getMethodString() == "HEAD") &&
        RequestHandler::isCgiRequest(filePath, location))
    {
        handleCgiRequest(request, location, filePath);
        _parser->reset();
        _parsedOffset = 0;
        return;
    }

    HttpResponse response = RequestHandler::handleRequest(request, location);
    buildFinalResponse(response, request);

    _parser->reset();
    _parsedOffset = 0;
}

void Client::buildParseErrorResponse(const HttpParser &parser)
{
    int errorCode = parser.getErrorCode();
    if (errorCode == 0)
        errorCode = STATUS_BAD_REQUEST;
    HttpResponse errorResp = HttpResponse::makeError(errorCode);
    errorResp.setConnection(false);
    _responseBuffer = errorResp.build();
}

size_t Client::findMatchingServerIndex(const std::vector<ServerConfig> &servers) const
{
    for (size_t i = 0; i < servers.size(); i++)
    {
        if (servers[i].getSocketFD() == _listenFd)
            return i;
    }
    return 0;
}

std::string Client::extractHostname(const HttpRequest &request) const
{
    std::string hostHeader = request.getHost();
    size_t colonPos = hostHeader.find(':');
    if (colonPos != std::string::npos)
        return hostHeader.substr(0, colonPos);
    return hostHeader;
}

HttpResponse Client::buildErrorWithCustomPage(int statusCode, const Location &location,
                                              bool keepAlive)
{
    HttpResponse response = HttpResponse::makeError(statusCode);
    response = RequestHandler::applyCustomErrorPage(response, location);
    response.setConnection(keepAlive);
    return response;
}

void Client::handleCgiRequest(HttpRequest &request, const Location &location,
                              const std::string &filePath)
{
    if (!RequestHandler::isMethodAllowed(request, location))
    {
        _responseBuffer = buildErrorWithCustomPage(STATUS_METHOD_NOT_ALLOWED, location,
                                                   request.isKeepAlive())
                              .build();
        return;
    }
    if (RequestHandler::isBodyTooLarge(request, location))
    {
        _responseBuffer = buildErrorWithCustomPage(STATUS_PAYLOAD_TOO_LARGE, location,
                                                   request.isKeepAlive())
                              .build();
        return;
    }

    HttpResponse errorResponse;
    if (CgiHandler::startCgi(request, location, filePath, request.getBodyRef(), _cgiProcess, errorResponse))
    {
        _cgiProcess.keepAlive = request.isKeepAlive();
        _state = CGI_PROCESSING;
        return;
    }

    errorResponse = RequestHandler::applyCustomErrorPage(errorResponse, location);
    errorResponse.setConnection(request.isKeepAlive());
    _responseBuffer = errorResponse.build();
}

void Client::buildFinalResponse(HttpResponse response, const HttpRequest &request)
{
    if (request.getMethodString() == "HEAD")
    {
        size_t bodyLength = response.getBody().length();
        response.setBody("");
        response.setContentLength(bodyLength);
    }
    if (request.getPath() == "/" && response.getStatus() == STATUS_OK)
        response.setHeader("Set-Cookie", "SESSIONID=root-session; Path=/");
    response.setConnection(request.isKeepAlive());
    _responseBuffer = response.build();
}

void Client::setSendOffset(size_t value)
{
    _sendOffset = value;
}

CgiProcess &Client::getCgiProcess()
{
    return _cgiProcess;
}

const CgiProcess &Client::getCgiProcess() const
{
    return _cgiProcess;
}

Config *Client::getConfig() const
{
    return _config;
}
