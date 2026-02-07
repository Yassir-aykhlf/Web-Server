#include "HttpParser.hpp"

HttpParser::HttpParser()
    : _state(PARSE_REQUEST_LINE)
    , _errorCode(0)
    , _contentLength(0)
    , _bodyBytesRead(0)
    , _chunkSize(0)
    , _chunkBytesRead(0)
    , _maxBodySize(DEFAULT_MAX_BODY_SIZE) {
}

HttpParser::~HttpParser() {}

// core finite state machine
HttpParser::ParseResult HttpParser::parse(const char* data, size_t len) {
    _buffer.append(data, len);
    while (true) {
        switch (_state) {
            case PARSE_REQUEST_LINE:
                if (parseRequestLine() == false)
                    return (_state == PARSE_ERROR) ? RESULT_ERROR : RESULT_INCOMPLETE;
                break;

            case PARSE_HEADERS:
                if (parseHeaders() == false)
                    return (_state == PARSE_ERROR) ? RESULT_ERROR : RESULT_INCOMPLETE;
                break;

            case PARSE_BODY:
                if (parseBody() == false)
                    return (_state == PARSE_ERROR) ? RESULT_ERROR : RESULT_INCOMPLETE;
                break;

            case PARSE_CHUNKED_SIZE:
                if (parseChunkSize() == false)
                    return (_state == PARSE_ERROR) ? RESULT_ERROR : RESULT_INCOMPLETE;
                break;

            case PARSE_CHUNKED_DATA:
                if (parseChunkData() == false)
                    return (_state == PARSE_ERROR) ? RESULT_ERROR : RESULT_INCOMPLETE;
                break;

            case PARSE_COMPLETE:
                return RESULT_COMPLETE;
            
            case PARSE_ERROR:
                return RESULT_ERROR;
        }
    }
}

bool HttpParser::parseRequestLine() {
    size_t pos = _buffer.find("\r\n");
    if (pos == std::string::npos) {
        return false;
    }
    std::string line = _buffer.substr(0, pos);
    _buffer.erase(0, pos + 2);
    std::vector<std::string> parts = split(line, ' ');
    if (parts.size() != 3) {
        setError(STATUS_BAD_REQUEST, "Invalid request line");
        return false;
    }
    _request.setMethodString(parts[0]);
    _request.setMethod(stringToMethod(parts[0]));
    std::string uri = parts[1];
    if (uri.length() > 8192) {
        setError(STATUS_URI_TOO_LONG, "URI too long");
        return false;
    }
    _request.setUri(uri);
    size_t queryPos = uri.find('?');
    if (queryPos != std::string::npos) {
        _request.setPath(urlDecode(uri.substr(0, queryPos)));
        _request.setQueryString(uri.substr(queryPos + 1));
    }
    else {
        _request.setPath(urlDecode(uri));
    }
    _request.setPath(normalizePath(_request.getPath()));
    _request.setVersion(parts[2]);
    if (parts[2] != "HTTP/1.0" && parts[2] != "HTTP/1.1") {
        setError(STATUS_BAD_REQUEST, "Unsupported HTTP version");
        return false;
    }
    _state = PARSE_HEADERS;
    return true;
}

bool HttpParser::parseHeaders() {
    while (true) {
        size_t pos = _buffer.find("\r\n");
        if (pos == std::string::npos)
            return false;
        std::string line = _buffer.substr(0, pos);
        _buffer.erase(0, pos + 2);
        if (line.empty()) {
            if (_request.getVersion() == "HTTP/1.1" && _request.getHost().empty()) {
                setError(STATUS_BAD_REQUEST, "Missing Host header");
                return false;
            }
            if (_request.isChunked()) {
                _state = PARSE_CHUNKED_SIZE;
            }
            else {
                _contentLength = _request.getContentLength();
                // Anti-DDos / overflow protection
                if (_contentLength > _maxBodySize) {
                    setError(STATUS_PAYLOAD_TOO_LARGE, "Request body too large");
                    return false;
                }
                if (_contentLength > 0) {
                    _state = PARSE_BODY;
                }
                else {
                    _state = PARSE_COMPLETE;
                }
            }
            return true;
        }
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            setError(STATUS_BAD_REQUEST, "Invalid header format");
            return false;
        }
        std::string name  = trim(line.substr(0, colonPos));
        std::string value = trim(line.substr(colonPos + 1));
        _request.setHeader(name, value);
    }
}

bool HttpParser::parseBody() {
    size_t remaining = _contentLength - _bodyBytesRead;
    size_t available = _buffer.length();
    size_t toRead = (available < remaining) ? available : remaining;
    if (toRead > 0) {
        _request.appendBody(_buffer.substr(0, toRead));
        _buffer.erase(0, toRead);
        _bodyBytesRead += toRead;
    }
    if (_bodyBytesRead >= _contentLength) {
        _state = PARSE_COMPLETE;
        return true;
    }
    return false;
}

bool HttpParser::parseChunkSize() {
    size_t pos = _buffer.find("\r\n");
    if (pos == std::string::npos) {
        return false;
    }
    std::string line = _buffer.substr(0, pos);
    _buffer.erase(0, pos + 2);
    std::istringstream iss(line);
    iss >> std::hex >> _chunkSize;
    if (_chunkSize == 0) {
        if (_buffer.length() >= 2 && _buffer.substr(0, 2) == "\r\n") {
            _buffer.erase(0, 2);
        }
        _state = PARSE_COMPLETE;
        return true;
    }
    if (_request.getBody().length() + _chunkSize > _maxBodySize) {
        setError(STATUS_PAYLOAD_TOO_LARGE, "Request body too large");
        return false;
    }
    _chunkBytesRead = 0;
    _state = PARSE_CHUNKED_DATA;
    return true;
}

bool HttpParser::parseChunkData() {
    size_t remaining = _chunkSize - _chunkBytesRead;
    size_t available = _buffer.length();
    size_t toRead = (available < remaining) ? available : remaining;
    if (toRead > 0) {
        _request.appendBody(_buffer.substr(0, toRead));
        _buffer.erase(0, toRead);
        _chunkBytesRead += toRead;
    }
    if (_chunkBytesRead >= _chunkSize) {
        if (_buffer.length() >= 2 && _buffer.substr(0, 2) == "\r\n") {
            _buffer.erase(0, 2);
        }
        _state = PARSE_CHUNKED_SIZE;
        return true;
    }
    return false;
}

void HttpParser::reset() {
    _request.clear();
    _state = PARSE_REQUEST_LINE;
    _buffer.clear();
    _contentLength = 0;
    _bodyBytesRead = 0;
    _chunkSize = 0;
    _chunkBytesRead = 0;
    _errorCode = 0;
}

HttpMethod HttpParser::stringToMethod(const std::string& method) {
    if (method == "GET")   return METHOD_GET;
    if (method == "POST")   return METHOD_POST;
    if (method == "DELETE")   return METHOD_DELETE;
    return METHOD_UNKNOWN;
}

void HttpParser::setError(int code, const std::string& message) {
    _state = PARSE_ERROR;
    _errorCode = code;
}

const HttpRequest& HttpParser::getRequest() const { return _request; }
HttpParser::ParserState HttpParser::getState() const { return _state; }
int  HttpParser::getErrorCode() const { return _errorCode; }
void HttpParser::setMaxBodySize(size_t size) { _maxBodySize = size; }