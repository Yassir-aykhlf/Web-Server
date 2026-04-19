#pragma once

#include <string>
#include <sys/types.h>

int createTcpSocket(const std::string &host, int port);
bool enableAddressReuse(int socketFd);
bool bindToAddress(int socketFd, const std::string &host, int port);
bool startListening(int socketFd);
void closeSocket(int socketFd);

struct ClientConnection
{
    int fd;
    std::string ipAddress;
    int port;
    ClientConnection() : fd(-1), port(0) {}
};

ClientConnection waitForClient(int listenerFd);
std::string receiveData(int socketFd, size_t maxBytes, bool &connectionClosed, bool &readError);
ssize_t sendData(int socketFd, const std::string &data, size_t offset);
