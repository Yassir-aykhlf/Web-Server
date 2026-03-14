#include "socket_utils.h"
#include "Logger.hpp"
#include "webserv.hpp"
#include <net/if.h>

int createTcpSocket(const std::string& host, int port) {
    struct addrinfo hints, *result = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    std::string portStr = intToString(port);
    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0 || result == NULL) {
        Logger::error("Failed to resolve address: " + host);
        return -1;
    }
    int fd = socket(result->ai_family, SOCK_STREAM, 0);
    freeaddrinfo(result);
    if (fd < 0) {
        Logger::error("Failed to create socket for " + host + ":" + portStr);
        return -1;
    }
    return fd;
}

bool enableAddressReuse(int socketFd) {
    int opt = 1;
    if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        Logger::error("Failed to enable address reuse on socket (fd: " + intToString(socketFd) + ")");
        return false;
    }
    return true;
}

bool bindToAddress(int socketFd, const std::string& host, int port) {
    struct addrinfo hints, *result = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    std::string portStr = intToString(port);
    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0 || result == NULL) {
        Logger::error("Failed to resolve address: " + host);
        return false;
    }
    if (result->ai_family == AF_INET6) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)result->ai_addr;
        if (addr6->sin6_scope_id == 0) {
            // Try common interface names
            unsigned int idx = if_nametoindex("eth0");
            if (idx == 0)
                idx = if_nametoindex("en0");
            if (idx == 0)
                idx = if_nametoindex("lo");
            if (idx == 0)
                Logger::warning("No valid network interface found for IPv6 link-local scope_id");
            addr6->sin6_scope_id = idx;
        }
    }
    if (bind(socketFd, result->ai_addr, result->ai_addrlen) < 0) {
        Logger::error("Failed to bind socket to " + host + ":" + portStr);
        freeaddrinfo(result);
        return false;
    }
    freeaddrinfo(result);
    return true;
}

bool startListening(int socketFd) {
    if (listen(socketFd, SOMAXCONN) < 0) {
        Logger::error("Failed to start listening on socket (fd: " + intToString(socketFd) + ")");
        return false;
    }
    return true;
}

void closeSocket(int socketFd) {
    if (socketFd >= 0)
        close(socketFd);
}

ClientConnection waitForClient(int listenerFd) {
    ClientConnection conn;
    struct sockaddr_storage clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    conn.fd = accept(listenerFd, (struct sockaddr*)&clientAddr, &clientLen);
    if (conn.fd < 0) {
        return conn;
    }
    char ipStr[INET6_ADDRSTRLEN];
    if (clientAddr.ss_family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)&clientAddr;
        inet_ntop(AF_INET, &addr4->sin_addr, ipStr, sizeof(ipStr));
        conn.port = ntohs(addr4->sin_port);
    } else {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&clientAddr;
        inet_ntop(AF_INET6, &addr6->sin6_addr, ipStr, sizeof(ipStr));
        conn.port = ntohs(addr6->sin6_port);
    }
    conn.ipAddress = ipStr;
    return conn;
}

std::string receiveData(int socketFd, size_t maxBytes, bool& connectionClosed) {
    connectionClosed = false;
    char buffer[BUFFER_SIZE];
    size_t toRead = maxBytes < sizeof(buffer) ? maxBytes : sizeof(buffer);
    ssize_t bytesRead = recv(socketFd, buffer, toRead, 0);
    if (bytesRead <= 0) {
        connectionClosed = true;
        return "";
    }
    return std::string(buffer, bytesRead);
}

ssize_t sendData(int socketFd, const std::string& data, size_t offset) {
    if (offset >= data.size())
        return -1;
    ssize_t sent = send(socketFd, data.c_str() + offset, data.size() - offset, 0);
    return sent;
}
