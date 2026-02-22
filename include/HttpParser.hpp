#pragma once

#include "webserv.hpp"
#include "HttpRequest.hpp"

class HttpParser {
public:
    enum ParserState {
        PARSE_REQUEST_LINE,
        PARSE_HEADERS,
        PARSE_BODY,
        PARSE_CHUNKED_SIZE,
        PARSE_CHUNKED_DATA,
        PARSE_COMPLETE,
        PARSE_ERROR
    };
    enum ParseResult {
        RESULT_INCOMPLETE,
        RESULT_COMPLETE,
        RESULT_ERROR
    };

    HttpParser();
    ~HttpParser();

    ParseResult parse(const char* data, size_t len);
    const HttpRequest& getRequest() const;
    ParserState getState() const;
    int getErrorCode() const;
    void reset();
    void setMaxBodySize(size_t size);

private:
    HttpRequest _request;
    ParserState _state;
    std::string _buffer;
    size_t _contentLength;
    size_t _bodyBytesRead;
    size_t _chunkSize;
    size_t _chunkBytesRead;
    size_t _maxBodySize;
    int _errorCode;
    std::string _errorMessage;
    bool parseRequestLine();
    bool parseHeaders();
    bool parseBody();
    bool parseChunkSize();
    bool parseChunkData();
    void setError(int code, const std::string& message);
    HttpMethod stringToMethod(const std::string& method);
};

