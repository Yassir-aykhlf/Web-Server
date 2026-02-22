#include "EventLoop.hpp"
#include "Client.hpp"
#include "webserv.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>

EventLoop::EventLoop() : _config(NULL), _running(false), _n_fds(0) {}

EventLoop::~EventLoop() {}

void EventLoop::setConfig(Config* config) {
    _config = config;
}

void EventLoop::removeClient(int index) {
    int fd = _pollfds[index].fd;
    Logger::info("Closing connection (fd: " + intToString(fd) + ")");
    delete _clients[fd];
    _clients.erase(fd);
    close(fd);
    _pollfds[index] = _pollfds.back();
    _pollfds.pop_back();
    _n_fds--;
}

void EventLoop::acceptNewConnection(int listenerFd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(listenerFd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        Logger::error("Failed to accept connection");
        return;
    }
    Logger::info("Accepted connection from "
        + std::string(inet_ntoa(client_addr.sin_addr)) + ":"
        + intToString(ntohs(client_addr.sin_port))
        + " (fd: " + intToString(client_fd) + ")");
    non_blocking(client_fd);

    struct pollfd client_pfd;
    client_pfd.fd = client_fd;
    client_pfd.events = POLLIN;
    client_pfd.revents = 0;
    _pollfds.push_back(client_pfd);

    _clients[client_fd] = new Client(client_fd, _config, listenerFd);
    _n_fds++;
}

void EventLoop::handleClientRead(int index) {
    int fd = _pollfds[index].fd;
    char buffer[1024];
    ssize_t bytes_read = recv(fd, buffer, sizeof(buffer), 0);

    if (bytes_read <= 0) {
        removeClient(index);
        return;
    }

    _clients[fd]->appendToRequestBuffer(std::string(buffer, bytes_read));
    Logger::debug("Received " + intToString(bytes_read) + " bytes from fd: " + intToString(fd));

    if (_clients[fd]->isRequestComplete()) {
        _clients[fd]->buildResponse();
        _clients[fd]->setState(WRITING);
        _pollfds[index].events = POLLOUT;
    }
}

void EventLoop::handleClientWrite(int index) {
    int fd = _pollfds[index].fd;
    Client* client = _clients[fd];
    const std::string& resp = client->getResponseBuffer();

    ssize_t sent = send(fd,
        resp.c_str() + client->getSendOffset(),
        resp.size() - client->getSendOffset(), 0);

    if (sent <= 0) {
        Logger::error("Failed to send data to (fd: " + intToString(fd) + ")");
        removeClient(index);
        return;
    }

    client->setSendOffset(client->getSendOffset() + sent);

    if (client->getSendOffset() >= resp.size()) {
        bool shouldClose = (resp.find("Connection: close") != std::string::npos);
        if (shouldClose) {
            removeClient(index);
        } else {
            client->clearResponseBuffer();
            client->clearRequestBuffer();
            client->setState(READING);
            client->setSendOffset(0);
            _pollfds[index].events = POLLIN;
        }
    }
}

void EventLoop::run() {
    if (!_config) {
        Logger::error("EventLoop: No configuration set");
        return;
    }
    _running = true;
    Logger::info("EventLoop started");

    // Register all listener sockets for polling
    std::vector<ServerConfig>& serverConfig = _config->getServerConfigs();
    for (size_t i = 0; i < serverConfig.size(); ++i) {
        int fd_socket = serverConfig[i].getSocketFD();
        if (fd_socket < 0) {
            Logger::error("Invalid socket fd for server on "
                + serverConfig[i].getHost() + ":" + intToString(serverConfig[i].getPort()));
            return;
        }
        _listenSockets.insert(fd_socket);
        struct pollfd pfd;
        pfd.fd = fd_socket;
        pfd.events = POLLIN;
        pfd.revents = 0;
        _pollfds.push_back(pfd);
        _clients[fd_socket] = NULL;
        _n_fds++;
    }

    // Main event loop
    while (_running) {
        int poll_count = poll(&_pollfds[0], _pollfds.size(), -1);
        if (poll_count < 0) {
            Logger::error("Poll error");
            cleanup();
            return;
        }

        for (int i = 0; i < _n_fds; i++) {
            bool isListener = (_listenSockets.count(_pollfds[i].fd) > 0);

            if (isListener) {
                if (_pollfds[i].revents & POLLIN)
                    acceptNewConnection(_pollfds[i].fd);
                continue;
            }

            if (_pollfds[i].revents & POLLIN) {
                handleClientRead(i);
                if (i >= _n_fds) break;
                if (_pollfds[i].revents & POLLOUT)
                    continue;
            }

            if (_pollfds[i].revents & POLLOUT) {
                handleClientWrite(i);
                if (i >= _n_fds) break;
            }
        }
    }

    Logger::info("EventLoop stopped");
}

const std::vector<struct pollfd> EventLoop::getListenFds() const {
    return _pollfds;
}

void EventLoop::addListenFd(struct pollfd pfd) {
    _pollfds.push_back(pfd);
}

int EventLoop::removeListenFd(int index) {
    close(_pollfds[index].fd);
    _pollfds.erase(_pollfds.begin() + index);
    return index - 1;
}

void EventLoop::cleanup() {
    for (size_t i = 0; i < _pollfds.size(); ++i) {
        close(_pollfds[i].fd);
    }
    _pollfds.clear();
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        delete it->second;
    }
    _clients.clear();
}

void EventLoop::stop() {
    _running = false;
}
