#include "Client.hpp"
#include "HttpParser.hpp"
#include "HttpResponse.hpp"
#include "RequestHandler.hpp"
#include "CgiHandler.hpp"
#include "ConfigRouter.hpp"
#include "Config.hpp"
#include "Logger.hpp"
#include <sstream>

// ── CgiProcess implementation ──

CgiProcess::CgiProcess()
    : pid(-1), pipeIn(-1), pipeOut(-1),
      bytesWritten(0), stdinDone(true),
      startTime(0), timeoutSec(DEFAULT_CGI_TIMEOUT_SECONDS), keepAlive(false) {}

void CgiProcess::reset() {
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

bool CgiProcess::isActive() const {
    return pid > 0;
}

// ── Client implementation ──

Client::Client(int fd, Config* config, int listenFd) : _fd(fd), _listenFd(listenFd), _requestBuffer(""), _responseBuffer(""), _sendOffset(0), _config(config) {
    _state = READING;
}

Client::~Client() {}

int Client::getFd() const {
    return _fd;
}

int Client::getListenFd() const {
    return _listenFd;
}

void Client::appendToRequestBuffer(const std::string& data) {
    _requestBuffer += data;
}

const std::string& Client::getRequestBuffer() const {
    return _requestBuffer;
}

void Client::clearRequestBuffer() {
    _requestBuffer.clear();
}

void Client::appendToResponseBuffer(const std::string& data) {
    _responseBuffer += data;
}

const std::string& Client::getResponseBuffer() const {
    return _responseBuffer;
}

void Client::clearResponseBuffer() {
    _responseBuffer.clear();
}

ClientState Client::getState() const {
    return _state;
}
void Client::setState(ClientState state) {
    _state = state;
}

size_t Client::findContentLength() const {
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

bool Client::isRequestComplete() const {
    size_t headerEnd = _requestBuffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return false;

    size_t contentLength = findContentLength();
    if (contentLength > 0) {
        size_t bodyStart = headerEnd + 4;
        return (_requestBuffer.length() - bodyStart) >= contentLength;
    }
    return true;
}

size_t Client::getSendOffset() const {
    return _sendOffset;
}

void Client::buildResponse() {
    HttpParser parser;
    HttpParser::ParseResult result = parser.parse(_requestBuffer.c_str(), _requestBuffer.length());

    if (result == HttpParser::RESULT_ERROR) {
        buildParseErrorResponse(parser);
        return;
    }

    const HttpRequest& request = parser.getRequest();

    if (!_config || _config->getServerConfigs().empty()) {
        buildFinalResponse(HttpResponse::makeError(STATUS_INTERNAL_SERVER_ERROR,
            _config ? "No server configuration" : "No configuration"), request);
        return;
    }

    std::vector<ServerConfig>& servers = _config->getServerConfigs();
    size_t serverIdx = findMatchingServerIndex(servers);
    std::string hostname = extractHostname(request);
    ConfigRouter router(servers[serverIdx], hostname);
    Location location = router.route(request.getPath());

    std::string filePath = RequestHandler::resolveFilePath(request, location);
    if (RequestHandler::isCgiRequest(filePath, location)) {
        handleCgiRequest(request, location, filePath);
        return;
    }

    HttpResponse response = RequestHandler::handleRequest(request, location);
    buildFinalResponse(response, request);
}

void Client::buildParseErrorResponse(const HttpParser& parser) {
    int errorCode = parser.getErrorCode();
    if (errorCode == 0) errorCode = STATUS_BAD_REQUEST;
    HttpResponse errorResp = HttpResponse::makeError(errorCode);
    errorResp.setConnection(false);
    _responseBuffer = errorResp.build();
}

size_t Client::findMatchingServerIndex(const std::vector<ServerConfig>& servers) const {
    for (size_t i = 0; i < servers.size(); i++) {
        if (servers[i].getSocketFD() == _listenFd)
            return i;
    }
    return 0;
}

std::string Client::extractHostname(const HttpRequest& request) const {
    std::string hostHeader = request.getHost();
    size_t colonPos = hostHeader.find(':');
    if (colonPos != std::string::npos)
        return hostHeader.substr(0, colonPos);
    return hostHeader;
}

HttpResponse Client::buildErrorWithCustomPage(int statusCode, const Location& location,
                                               bool keepAlive) {
    HttpResponse response = HttpResponse::makeError(statusCode);
    response = RequestHandler::applyCustomErrorPage(response, location);
    response.setConnection(keepAlive);
    return response;
}

void Client::handleCgiRequest(const HttpRequest& request, const Location& location,
                               const std::string& filePath) {
    if (!RequestHandler::fileExists(filePath)) {
        _responseBuffer = buildErrorWithCustomPage(STATUS_NOT_FOUND, location,
                                                    request.isKeepAlive()).build();
        return;
    }
    if (!RequestHandler::isMethodAllowed(request, location)) {
        _responseBuffer = buildErrorWithCustomPage(STATUS_METHOD_NOT_ALLOWED, location,
                                                    request.isKeepAlive()).build();
        return;
    }
    if (RequestHandler::isBodyTooLarge(request, location)) {
        _responseBuffer = buildErrorWithCustomPage(STATUS_PAYLOAD_TOO_LARGE, location,
                                                    request.isKeepAlive()).build();
        return;
    }

    HttpResponse errorResponse;
    if (CgiHandler::startCgi(request, location, filePath, _cgiProcess, errorResponse)) {
        _cgiProcess.keepAlive = request.isKeepAlive();
        _state = CGI_PROCESSING;
        return;
    }

    errorResponse = RequestHandler::applyCustomErrorPage(errorResponse, location);
    errorResponse.setConnection(request.isKeepAlive());
    _responseBuffer = errorResponse.build();
}

void Client::buildFinalResponse(HttpResponse response, const HttpRequest& request) {
    response.setConnection(request.isKeepAlive());
    _responseBuffer = response.build();
}

void Client::setSendOffset(size_t value) {
    _sendOffset = value;
}

CgiProcess& Client::getCgiProcess() {
    return _cgiProcess;
}

const CgiProcess& Client::getCgiProcess() const {
    return _cgiProcess;
}

Config* Client::getConfig() const {
    return _config;
}
