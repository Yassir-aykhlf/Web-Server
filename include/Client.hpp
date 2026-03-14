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

enum ClientState {
    READING,
    WRITING,
    CGI_PROCESSING,
};

struct CgiProcess {
    pid_t       pid;
    int         pipeIn;         // fd to write request body to CGI stdin
    int         pipeOut;        // fd to read CGI stdout from
    std::string outputBuffer;   // accumulated CGI output
    std::string bodyToWrite;    // request body still to send to CGI
    size_t      bytesWritten;   // how much of bodyToWrite has been sent
    bool        stdinDone;      // true once all body data sent & pipe closed
    time_t      startTime;
    int         timeoutSec;
    bool        keepAlive;      // from the original request
    Location    location;       // for custom error pages

    CgiProcess();
    void reset();
    bool isActive() const;
};

class Client {
    public:
        Client(int fd, Config* config, int listenFd);
        ~Client();
        int getFd() const;
        int getListenFd() const;
        void appendToRequestBuffer(const std::string& data);
        const std::string& getRequestBuffer() const;
        void clearRequestBuffer();
        void appendToResponseBuffer(const std::string& data);
        const std::string& getResponseBuffer() const;
        void clearResponseBuffer();
        ClientState getState() const;
        void setState(ClientState state);
        bool isRequestComplete() const;
        void buildResponse();
        void setSendOffset(size_t value);
        size_t getSendOffset() const;

        // CGI async support
        CgiProcess& getCgiProcess();
        const CgiProcess& getCgiProcess() const;
        Config* getConfig() const;

    private:
        int _fd;
        int _listenFd;
        std::string _requestBuffer;
        std::string _responseBuffer;
        ClientState _state;
        size_t _sendOffset;
        Config* _config;
        CgiProcess _cgiProcess;

        // Response building helpers
        void buildParseErrorResponse(const HttpParser& parser);
        size_t findMatchingServerIndex(const std::vector<ServerConfig>& servers) const;
        std::string extractHostname(const HttpRequest& request) const;
        HttpResponse buildErrorWithCustomPage(int statusCode, const Location& location,
                                               bool keepAlive);
        void handleCgiRequest(const HttpRequest& request, const Location& location,
                               const std::string& filePath);
        void buildFinalResponse(HttpResponse response, const HttpRequest& request);
        size_t findContentLength() const;
    };

