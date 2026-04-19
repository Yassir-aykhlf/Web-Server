#pragma once

#include <string>
#include <vector>
#include <unistd.h>
#include <ctime>
#include <sys/types.h>
#include "Location.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ServerConfig.hpp"

class Config;
class HttpParser;

enum ClientState
{
    READING,
    WRITING,
    CGI_PROCESSING,
};

struct CgiProcess
{
    pid_t pid;
    int pipeIn;
    int pipeOut;
    std::string outputBuffer;
    std::string bodyToWrite;
    size_t bytesWritten;
    bool stdinDone;
    time_t startTime;
    int timeoutSec;
    bool keepAlive;
    Location location;

    CgiProcess();
    void reset();
    bool isActive() const;
};

class Client
{
public:
    Client(int fd, Config *config, int listenFd);
    ~Client();
    int getFd() const;
    int getListenFd() const;
    void appendToRequestBuffer(const std::string &data);
    void setRequestBuffer(const std::string &data);
    const std::string &getRequestBuffer() const;
    void clearRequestBuffer();
    void appendToResponseBuffer(const std::string &data);
    void appendToResponseBuffer(const char *data, size_t len);
    const std::string &getResponseBuffer() const;
    void clearResponseBuffer();
    ClientState getState() const;
    void setState(ClientState state);
    bool isRequestComplete() const;
    bool hasIncompleteRequest() const;
    void buildResponse();
    void setSendOffset(size_t value);
    size_t getSendOffset() const;

    CgiProcess &getCgiProcess();
    const CgiProcess &getCgiProcess() const;
    Config *getConfig() const;

private:
    int _fd;
    int _listenFd;
    std::string _requestBuffer;
    std::string _responseBuffer;
    ClientState _state;
    size_t _sendOffset;
    Config *_config;
    CgiProcess _cgiProcess;
    std::string _cgiBodyBuffer;
    HttpParser *_parser;
    size_t _parsedOffset;

    void buildParseErrorResponse(const HttpParser &parser);
    size_t findMatchingServerIndex(const std::vector<ServerConfig> &servers) const;
    std::string extractHostname(const HttpRequest &request) const;
    HttpResponse buildErrorWithCustomPage(int statusCode, const Location &location,
                                          bool keepAlive);
    void handleCgiRequest(HttpRequest &request, const Location &location,
                          const std::string &filePath);
    void buildFinalResponse(HttpResponse response, const HttpRequest &request);
    size_t findContentLength() const;
};
