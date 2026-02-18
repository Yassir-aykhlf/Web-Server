/*
Encapsulates the state of a single client connection.
It tracks the connection lifecycle state,
manages read/write buffers,
holds the pending Request/Response objects,
and tracks active CGI processes for that client.
*/

#include "Client.hpp"
#include <sstream>

Client::Client(int fd) : _fd(fd),  _requestBuffer(""), _responseBuffer(""), _sendOffset(0) {
    _state = READING;
}

Client::~Client() {}

int Client::getFd() const {
    return _fd;
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
    return _requestBuffer.find("\r\n\r\n") != std::string::npos;
}

std::string integerToString(int value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

size_t Client::getSendOffset() const {
    return _sendOffset;
}

void Client::buildResponse() {
    // For simplicity, always respond with a 200 OK and a fixed body
    std::string body = "<html><body><h1>Hello, World!</h1></body></html>";
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + integerToString(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" +
        body;
    _responseBuffer = response;
}

void Client::setSendOffset(size_t value) {
    _sendOffset = value;
}
