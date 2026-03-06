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
      startTime(0), timeoutSec(30), keepAlive(false) {}

void CgiProcess::reset() {
    pid = -1;
    pipeIn = -1;
    pipeOut = -1;
    outputBuffer.clear();
    bodyToWrite.clear();
    bytesWritten = 0;
    stdinDone = true;
    startTime = 0;
    timeoutSec = 30;
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

bool Client::isRequestComplete() const {
    // Simple check: if we have received a double CRLF, consider the request complete
    size_t headerEnd = _requestBuffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return false;

    // Check for Content-Length to see if body is complete
    std::string lowerBuf = toLower(_requestBuffer.substr(0, headerEnd));
    size_t clPos = lowerBuf.find("content-length:");
    if (clPos != std::string::npos) {
        size_t valStart = clPos + 15; // length of "content-length:"
        size_t lineEnd = lowerBuf.find("\r\n", valStart);
        if (lineEnd == std::string::npos)
            lineEnd = lowerBuf.length();
        std::string clValue = trim(_requestBuffer.substr(valStart, lineEnd - valStart));
        size_t contentLength = static_cast<size_t>(stringToInt(clValue));
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
        int errorCode = parser.getErrorCode();
        if (errorCode == 0) errorCode = STATUS_BAD_REQUEST;
        HttpResponse errorResp = HttpResponse::makeError(errorCode);
        errorResp.setConnection(false);
        _responseBuffer = errorResp.build();
        return;
    }

    const HttpRequest& request = parser.getRequest();

    // Find the matching server config based on host header
    HttpResponse response;

    if (_config) {
        std::vector<ServerConfig>& servers = _config->getServerConfigs();
        if (!servers.empty()) {
            // Find the server config that owns the listener socket
            size_t serverIdx = 0;
            for (size_t i = 0; i < servers.size(); i++) {
                if (servers[i].getSocketFD() == _listenFd) {
                    serverIdx = i;
                    break;
                }
            }

            // Find best matching server by Host header within this server config
            std::string hostHeader = request.getHost();
            // Strip port from host header
            size_t colonPos = hostHeader.find(':');
            std::string hostname = (colonPos != std::string::npos) ?
                hostHeader.substr(0, colonPos) : hostHeader;

            ConfigRouter router(servers[serverIdx], hostname);
            Location location = router.route(request.getPath());

            // Check if this is a CGI request — if so, start it asynchronously
            std::string filePath = RequestHandler::resolveFilePath(request, location);
            if (RequestHandler::isCgiRequest(filePath, location)) {
                if (!RequestHandler::fileExists(filePath)) {
                    response = HttpResponse::makeError(STATUS_NOT_FOUND);
                    response = RequestHandler::applyCustomErrorPage(response, location);
                    response.setConnection(request.isKeepAlive());
                    _responseBuffer = response.build();
                    return;
                }

                // Check method allowed and body size before starting CGI
                if (!RequestHandler::isMethodAllowed(request, location)) {
                    response = HttpResponse::makeError(STATUS_METHOD_NOT_ALLOWED);
                    response = RequestHandler::applyCustomErrorPage(response, location);
                    response.setConnection(request.isKeepAlive());
                    _responseBuffer = response.build();
                    return;
                }

                size_t maxBody = RequestHandler::parseBodySize(location.getStringValue("client_max_body_size"));
                if (request.getBody().length() > maxBody) {
                    response = HttpResponse::makeError(STATUS_PAYLOAD_TOO_LARGE);
                    response = RequestHandler::applyCustomErrorPage(response, location);
                    response.setConnection(request.isKeepAlive());
                    _responseBuffer = response.build();
                    return;
                }

                // Start CGI asynchronously
                HttpResponse errorResponse;
                if (CgiHandler::startCgi(request, location, filePath, _cgiProcess, errorResponse)) {
                    _cgiProcess.keepAlive = request.isKeepAlive();
                    _state = CGI_PROCESSING;
                    return; // EventLoop will handle the rest
                } else {
                    // CGI failed to start — return the error
                    errorResponse = RequestHandler::applyCustomErrorPage(errorResponse, location);
                    errorResponse.setConnection(request.isKeepAlive());
                    _responseBuffer = errorResponse.build();
                    return;
                }
            }

            response = RequestHandler::handleRequest(request, location);
        } else {
            response = HttpResponse::makeError(STATUS_INTERNAL_SERVER_ERROR, "No server configuration");
        }
    } else {
        response = HttpResponse::makeError(STATUS_INTERNAL_SERVER_ERROR, "No configuration");
    }

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
