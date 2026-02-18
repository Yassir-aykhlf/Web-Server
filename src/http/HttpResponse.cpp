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
    std::ostringstream response;
    response << HTTP_VERSION << " " << _statusCode << " " << getStatusText(_statusCode) << "\r\n";
    
    // for (const auto& header : _headers) {  
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
        response << it->first << ": " << it->second << "\r\n";
    }
    if (_headers.find("Content-Length") == _headers.end()) {
        response << "Content-Length: " << _body.length() << "\r\n";
    }
    response << "\r\n";
    response << _body;
    return response.str();
}

HttpResponse HttpResponse::makeError(int code, const std::string& message) {
    HttpResponse response;
    response.setStatus(code);
    response.setContentType("text/html");
    std::string body =  "<!DOCTYPE html>\n"
                        "<html>\n"
                        "<head><title>" + intToString(code) + " " + getStatusText(code) + "</title></head>\n"
                        "<body>\n"
                        "<center><h1>" + intToString(code) + " " + getStatusText(code) + "</h1></center>\n";
    if (!message.empty()) {
        body += "<center><p>" + message + "</p></center>\n";
    }
    body += "<hr><center>" SERVER_NAME "</center>\n"
            "</body>\n"
            "</html>\n";
    response.setBody(body);
    return response;
}

HttpResponse HttpResponse::makeRedirect(int code, const std::string& url) {
    HttpResponse response;
    response.setStatus(code);
    response.setLocation(url);
    response.setContentType("text/html");
    std::string body =  "<!DOCTYPE html>\n"
                        "<html>\n"
                        "<head><title>" + intToString(code) + " " + getStatusText(code) + "</title></head>\n"
                        "<body>\n"
                        "<center><h1>" + intToString(code) + " " + getStatusText(code) + "</h1></center>\n"
                        "<center><p>Redirecting to <a href=\"" + url + "\">" + url + "</a></p></center>\n"
                        "</body>\n"
                        "</html>\n";
    response.setBody(body);
    return response;    
}