#include "HttpRequest.hpp"

HttpRequest::HttpRequest()
    :   _method(METHOD_UNKNOWN)
    ,   _version("HTTP/1.1") {
}

HttpRequest::HttpRequest(const HttpRequest& other) {
    *this = other;
}

HttpRequest& HttpRequest::operator=(const HttpRequest& other) {
    if (this != &other) {
        _method = other._method;
        _methodStr = other._methodStr;
        _uri = other._uri;
        _path = other._path;
        _query = other._query;
        _version = other._version;
        _headers = other._headers;
        _body = other._body;
    }
    return *this;
}

HttpRequest::~HttpRequest() {}

HttpMethod HttpRequest::getMethod() const { return _method; }
const std::string& HttpRequest::getMethodString() const { return _methodStr; }
const std::string& HttpRequest::getUri() const { return _uri; }
const std::string& HttpRequest::getPath() const { return _path; }
const std::string& HttpRequest::getQueryString() const { return _query; }
const std::string& HttpRequest::getVersion() const { return _version; }
const std::string& HttpRequest::getBody() const { return _body; }
const std::map<std::string, std::string>& HttpRequest::getHeaders() const { return _headers; }

void HttpRequest::setMethod(HttpMethod method) { _method = method; }
void HttpRequest::setMethodString(const std::string& method) { _methodStr = method; }
void HttpRequest::setUri(const std::string& uri) { _uri = uri; }
void HttpRequest::setPath(const std::string& path) { _path = path; }
void HttpRequest::setQueryString(const std::string& query) { _query = query; }
void HttpRequest::setVersion(const std::string& version) { _version = version; }


void HttpRequest::setBody(const std::string& body) { 
    _body = body;
}
void HttpRequest::appendBody(const std::string& chunk) { 
    _body += chunk;
}

void HttpRequest::setHeader(const std::string& key, const std::string& value) {
    _headers[toLower(key) = value];
}

std::string HttpRequest::getHeader(const std::string& name) const {
    std::string lowerName = toLower(name);
    std::map<std::string, std::string>::const_iterator it = _headers.find(lowerName);
    if (it != _headers.end())
        return it->second;
    return "";
}

const std::string& HttpRequest::getHost() const {
    static std::string empty;
    std::map<std::string, std::string>::const_iterator it = _headers.find("host");
    if (it != _headers.end())
        return it->second;
    return empty;
}

size_t HttpRequest::getContentLength() const {
    std::string value = getHeader("content-length");
    if (value.empty())
        return 0;
    return static_cast<size_t>(stringToInt(value));
}

bool HttpRequest::isChunked() const {
    std::string transfer_encoding = getHeader("transfer-encoding");
    return toLower(transfer_encoding).find("chunked") != std::string::npos;
}

bool HttpRequest::isKeepAlive() const {
    std::string connection = toLower(getHeader("connection"));
    if (_version == "HTTP/1.1")
        return connection != "close";
    else
        return connection == "keep-alive";
}

void HttpRequest::clear() {
    _method = METHOD_UNKNOWN;
    _version = "HTTP/1.1";
    _methodStr.clear();
    _uri.clear();
    _path.clear();
    _query.clear();
    _headers.clear();
    _body.clear();
}