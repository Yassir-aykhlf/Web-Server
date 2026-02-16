#include "HttpResponse.hpp"

HttpResponse::HttpResponse()
    : _statusCode(200) {
    _headers["Server"] = SERVER_NAME;
    _headers["Date"] = getCurrentTime();
}

HttpResponse::HttpResponse(const HttpResponse& other) {
    *this = other;
}

HttpResponse& HttpResponse::operator=(const HttpResponse& other) {
    if (this != &other) {
        _statusCode = other._statusCode;
        _headers = other._headers;
        _body = other._body;
    }
    return *this;
}

HttpResponse::~HttpResponse() {}

void    HttpResponse::setStatus(int code) { _statusCode = code; }
void    HttpResponse::setHeader(const std::string& name, const std::string& value) {_headers[name] = value;}
void    HttpResponse::setBody(const std::string& body) { _body = body; }
void    HttpResponse::appendBody(const std::string& data) { _body += data; }

int     HttpResponse::getStatus() const { return _statusCode; }
const   std::string& HttpResponse::getBody() const { return _body; }

std::string HttpResponse::getHeader(const std::string& name) const {
    std::map<std::string, std::string>::const_iterator it = _headers.find(name);
    if (it != _headers.end())
        return it->second;
    return "";
}

void HttpResponse::setContentType(const std::string& type) {
    _headers["Content-Type"] = type;
}

void HttpResponse::setContentLength(size_t length) {
    _headers["Content-Length"] = longToString(static_cast<long>(length));
}

void HttpResponse::setLocation(const std::string& url) {
    _headers["Location"] = url;
}

void HttpResponse::setConnection(bool keepAlive) {
    _headers["Connection"] = keepAlive ? "keep-alive" : "close";
}

void HttpResponse::clear() {
    _statusCode = 200;
    _headers.clear();
    _body.clear();
    _headers["Server"] = SERVER_NAME;
    _headers["Date"] = getCurrentTime();
}

std::string HttpResponse::build() const {
    // TODO: build status line, headers, and body into a complete HTTP response string
}

HttpResponse HttpResponse::makeError(int code, const std::string& message) {
    // TODO: create a simple error response with the given status code and message
}

HttpResponse HttpResponse::makeRedirect(int code, const std::string& url) {
    // TODO: create a simple redirect response with the given status code and URL
}