#pragma once

#include <string>
#include <unistd.h>

class Config;
class HttpParser;

enum ClientState {
    READING,
    WRITING,
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
        void updateRequestBuffer(int len);
        bool isRequestComplete() const;
        void buildResponse();
        void setSendOffset(size_t value);
        size_t getSendOffset() const;

    private:
        int _fd;
        int _listenFd;
        std::string _requestBuffer;
        std::string _responseBuffer;
        ClientState _state;
        size_t _sendOffset;
        Config* _config;
    };

