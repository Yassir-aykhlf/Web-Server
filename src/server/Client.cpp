#include "Client.hpp"
#include "HttpParser.hpp"
#include "HttpResponse.hpp"
#include "RequestHandler.hpp"
#include "ConfigRouter.hpp"
#include "Config.hpp"
#include "Logger.hpp"
#include <sstream>

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

// Remove processed data from the request buffer
// len(headers) + 4 + len(body)
void Client::updateRequestBuffer(int len) {
    _requestBuffer = _requestBuffer.substr(len);
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
