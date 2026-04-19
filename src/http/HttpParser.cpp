#include "HttpParser.hpp"
#include <limits>

HttpParser::HttpParser()
    : _state(PARSE_REQUEST_LINE), _contentLength(0), _bodyBytesRead(0), _chunkSize(0), _chunkBytesRead(0), _maxBodySize(std::numeric_limits<size_t>::max()), _errorCode(0)
{
}

HttpParser::~HttpParser() {}

// core finite state machine
HttpParser::ParseResult HttpParser::parse(const char *data, size_t len)
{
    _buffer.append(data, len);
    while (true)
    {
        switch (_state)
        {
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

bool HttpParser::parseRequestLine()
{
    size_t pos = _buffer.find("\r\n");
    if (pos == 0)
    {
        _buffer.erase(0, 2);
        return false;
    }
    if (pos == std::string::npos)
    {
        return false;
    }
    std::string line = trim(_buffer.substr(0, pos));
    _buffer.erase(0, pos + 2);
    if (line.empty())
        return false;
    std::vector<std::string> parts = split(line, ' ');
    if (parts.size() != 3)
    {
        setError(STATUS_BAD_REQUEST, "Invalid request line");
        return false;
    }
    _request.setMethodString(parts[0]);
    _request.setMethod(stringToMethod(parts[0]));
    if (!extractAndValidateUri(parts[1]))
        return false;
    _request.setVersion(parts[2]);
    if (!isSupportedHttpVersion(parts[2]))
    {
        setError(STATUS_HTTP_VERSION_NOT_SUPPORTED, "Unsupported HTTP version");
        return false;
    }
    _state = PARSE_HEADERS;
    return true;
}

bool HttpParser::extractAndValidateUri(const std::string &uri)
{
    if (uri.length() > MAX_URI_LENGTH)
    {
        setError(STATUS_URI_TOO_LONG, "URI too long");
        return false;
    }
    _request.setUri(uri);
    splitUriPathAndQuery(uri);
    if (_request.getPath().find('\0') != std::string::npos ||
        _request.getQueryString().find('\0') != std::string::npos)
    {
        setError(STATUS_BAD_REQUEST, "Invalid URI");
        return false;
    }
    _request.setPath(normalizePath(_request.getPath()));
    return true;
}

void HttpParser::splitUriPathAndQuery(const std::string &uri)
{
    size_t queryPos = uri.find('?');
    if (queryPos != std::string::npos)
    {
        _request.setPath(urlDecode(uri.substr(0, queryPos)));
        _request.setQueryString(uri.substr(queryPos + 1));
    }
    else
    {
        _request.setPath(urlDecode(uri));
    }
}

bool HttpParser::isSupportedHttpVersion(const std::string &version)
{
    return version == "HTTP/1.0" || version == "HTTP/1.1";
}

bool HttpParser::parseHeaders()
{
    while (true)
    {
        size_t pos = _buffer.find("\r\n");
        if (pos == std::string::npos)
            return false;
        std::string line = _buffer.substr(0, pos);
        _buffer.erase(0, pos + 2);
        if (line.empty())
        {
            if (_request.getVersion() == "HTTP/1.1" && _request.getHost().empty())
            {
                setError(STATUS_BAD_REQUEST, "Missing Host header");
                return false;
            }
            return determineBodyParsingStrategy();
        }
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos)
        {
            setError(STATUS_BAD_REQUEST, "Invalid header format");
            return false;
        }
        std::string name = trim(line.substr(0, colonPos));
        std::string value = trim(line.substr(colonPos + 1));
        _request.setHeader(name, value);
    }
}

bool HttpParser::determineBodyParsingStrategy()
{
    if (_request.isChunked())
    {
        _state = PARSE_CHUNKED_SIZE;
        return true;
    }
    _contentLength = _request.getContentLength();
    if (_contentLength > 0)
    {
        if (_contentLength > _maxBodySize)
        {
            setError(STATUS_PAYLOAD_TOO_LARGE, "Request body too large");
            return false;
        }
        _request.reserveBody(_contentLength);
        _state = PARSE_BODY;
    }
    else
        _state = PARSE_COMPLETE;
    return true;
}

bool HttpParser::parseBody()
{
    size_t remaining = _contentLength - _bodyBytesRead;
    size_t available = _buffer.length();
    size_t toRead = (available < remaining) ? available : remaining;
    if (toRead > 0)
    {
        if (_bodyBytesRead + toRead > _maxBodySize)
        {
            setError(STATUS_PAYLOAD_TOO_LARGE, "Request body too large");
            return false;
        }
        _request.appendBody(_buffer.data(), toRead);
        _buffer.erase(0, toRead);
        _bodyBytesRead += toRead;
    }
    if (_bodyBytesRead >= _contentLength)
    {
        _state = PARSE_COMPLETE;
        return true;
    }
    return false;
}

bool HttpParser::parseChunkSize()
{
    size_t pos = _buffer.find("\r\n");
    if (pos == std::string::npos)
    {
        return false;
    }
    std::string line = _buffer.substr(0, pos);
    _buffer.erase(0, pos + 2);
    std::istringstream iss(line);
    iss >> std::hex >> _chunkSize;
    if (_chunkSize > (_maxBodySize - _bodyBytesRead))
    {
        setError(STATUS_PAYLOAD_TOO_LARGE, "Request body too large");
        return false;
    }
    if (_chunkSize == 0)
    {
        if (_buffer.length() >= 2 && _buffer.substr(0, 2) == "\r\n")
        {
            _buffer.erase(0, 2);
        }
        _state = PARSE_COMPLETE;
        return true;
    }
    _chunkBytesRead = 0;
    _state = PARSE_CHUNKED_DATA;
    return true;
}

bool HttpParser::parseChunkData()
{
    size_t remaining = _chunkSize - _chunkBytesRead;
    size_t available = _buffer.length();
    size_t toRead = (available < remaining) ? available : remaining;
    if (toRead > 0)
    {
        if (_bodyBytesRead + toRead > _maxBodySize)
        {
            setError(STATUS_PAYLOAD_TOO_LARGE, "Request body too large");
            return false;
        }
        _request.appendBody(_buffer.data(), toRead);
        _buffer.erase(0, toRead);
        _chunkBytesRead += toRead;
        _bodyBytesRead += toRead;
    }
    if (_chunkBytesRead >= _chunkSize)
    {
        if (_buffer.length() < 2)
            return false;
        if (_buffer.substr(0, 2) != "\r\n")
        {
            setError(STATUS_BAD_REQUEST, "Invalid chunk delimiter");
            return false;
        }
        _buffer.erase(0, 2);
        _state = PARSE_CHUNKED_SIZE;
        return true;
    }
    return false;
}

void HttpParser::reset()
{
    _request.clear();
    _state = PARSE_REQUEST_LINE;
    _buffer.clear();
    _contentLength = 0;
    _bodyBytesRead = 0;
    _chunkSize = 0;
    _chunkBytesRead = 0;
    _errorCode = 0;
}

HttpMethod HttpParser::stringToMethod(const std::string &method)
{
    if (method == "GET")
        return METHOD_GET;
    if (method == "POST")
        return METHOD_POST;
    if (method == "DELETE")
        return METHOD_DELETE;
    return METHOD_UNKNOWN;
}

void HttpParser::setError(int code, const std::string &message)
{
    _state = PARSE_ERROR;
    _errorCode = code;
    _errorMessage = message;
}

const HttpRequest &HttpParser::getRequest() const { return _request; }
HttpRequest &HttpParser::getRequestMutable() { return _request; }
const std::string &HttpParser::getRemainingBuffer() const { return _buffer; }
HttpParser::ParserState HttpParser::getState() const { return _state; }
int HttpParser::getErrorCode() const { return _errorCode; }
void HttpParser::setMaxBodySize(size_t size) { _maxBodySize = size; }