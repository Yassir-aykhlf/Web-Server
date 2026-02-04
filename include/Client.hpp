#ifndef CLIENT_HPP
#define CLIENT_HPP
#include "EventLoop.hpp"

enum ClientState {
    READING,
    WRITING,
};

class Client {
    public:
        Client(int fd);
        ~Client();
        int getFd() const;
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
        std::string _requestBuffer;
        std::string _responseBuffer;
        ClientState _state;
        size_t _sendOffset;
    };

#endif
