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
    // TODO
}

bool HttpParser::parseHeaders() {
    // TODO
}

bool HttpParser::parseBody() {
    // TODO
}

bool HttpParser::parseChunkSize() {
    // TODO
}

bool HttpParser::parseChunkData() {
    // TODO
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

void HttpParser::setError(int code, const std::string& message) {
    _state = PARSE_ERROR;
    _errorCode = code;
}

const HttpRequest& HttpParser::getRequest() const { return _request; }
HttpParser::ParserState HttpParser::getState() const { return _state; }
int  HttpParser::getErrorCode() const { return _errorCode; }
void HttpParser::setMaxBodySize(size_t size) { _maxBodySize = size; }