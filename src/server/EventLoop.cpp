/*
The heart of the non-blocking architecture.
It implements the Reactor pattern using poll().
It monitors all file descriptors (listeners, clients, CGI pipes)
and dispatches events (POLLIN/POLLOUT) to the appropriate read/write handlers.
*/

#include "EventLoop.hpp"
#include "Client.hpp"
#include "webserv.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>

EventLoop::EventLoop() : _config(NULL), _running(false), _n_fds(0) {
    // _pollfds and _clients are default constructed
}

EventLoop::~EventLoop() {}

void EventLoop::setConfig(Config* config) {
    _config = config;
}

void EventLoop::run() {
    if (!_config) {
        Logger::error("EventLoop: No configuration set");
        return;
    }
    _running = true;
    Logger::info("EventLoop started");

    std::vector<ServerConfigue> serverConfig = _config->getServerConfigues();
    vector<int> fd_sockets;
    // Main event loop
    // memset(_pollfds, 0, sizeof(_pollfds));
    std::set<int> _listenSockets;
    for (size_t i = 0; i < serverConfig.size(); ++i) {
        int fd_socket = serverConfig[i].getSocketFD();
        if (fd_socket < 0) {
            Logger::error("Invalid socket fd for server on " + serverConfig[i].getHost() + ":" + intToString(serverConfig[i].getPort()));
            return;
        }
        fd_sockets.push_back(fd_socket);
        _listenSockets.insert(fd_socket);
        struct pollfd pfd;
        pfd.fd = fd_socket;
        pfd.events = POLLIN;
        pfd.revents = 0;
        _pollfds.push_back(pfd); // Use push_back for empty vector
        _clients[fd_socket] = NULL; // Mark listener fds with NULL client
        _n_fds++;
    }

    // Polling loop
    //serverConfig.getListenFds();
    while (_running) {
        int poll_count = poll(&_pollfds[0], _pollfds.size(), -1);
        // int poll_count = poll(_pollfds, _n_fds, -1);
        if (poll_count < 0) {
            Logger::error("Poll error");
            cleanup();
            return;
        }

        // Check existing connections for data
        for (int i = 0; i < _n_fds; i++) {
            // Check if this fd is a listener or a client
            if (_listenSockets.count(_pollfds[i].fd) > 0) {
                        // Check for new connections
                if (_pollfds[i].revents & POLLIN) {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(_pollfds[i].fd, (struct sockaddr*)&client_addr, &client_len);
                    if (client_fd < 0) {
                        Logger::error("Failed to accept connection");
                        continue;
                    }
                    Logger::info("Accepted connection from " + std::string(inet_ntoa(client_addr.sin_addr)) + ":" + intToString(ntohs(client_addr.sin_port)) + " (fd: " + intToString(client_fd) + ")");
                    non_blocking(client_fd);
                    
                    struct pollfd client_pfd;
                    client_pfd.fd = client_fd;
                    client_pfd.events = POLLIN;
                    client_pfd.revents = 0;
                    _pollfds.push_back(client_pfd);
                    
                    _clients[client_fd] = new Client(client_fd);
                    _n_fds++;
                }
                continue; // Skip listener fds
            }
            // Handle client events
            if (_pollfds[i].revents & POLLIN) {
                char buffer[1024];
                ssize_t bytes_read = recv(_pollfds[i].fd, buffer, sizeof(buffer), 0);
                if (bytes_read <= 0) {
                    Logger::info("Closing connection (fd: " + intToString(_pollfds[i].fd) + ")");
                    delete _clients[_pollfds[i].fd];
                    _clients.erase(_pollfds[i].fd);
                    close(_pollfds[i].fd);
                    
                    _pollfds[i] = _pollfds.back();
                    _pollfds.pop_back();
                    _n_fds--;
                    i--;
                } else {
                    
                    Logger::info("Received data from (fd: " + intToString(_pollfds[i].fd) + "): " + std::string(buffer, bytes_read));
                    _clients[_pollfds[i].fd]->appendToRequestBuffer(std::string(buffer, bytes_read));
                    if (_clients[_pollfds[i].fd]->isRequestComplete()) {
                        _clients[_pollfds[i].fd]->buildResponse();
                        _clients[_pollfds[i].fd]->setState(WRITING);
                        _pollfds[i].events = POLLOUT;
                    }
                }
            }

            if (_pollfds[i].revents & POLLOUT)
            {
                // Handle writable event
                const std::string& resp = _clients[_pollfds[i].fd]->getResponseBuffer();
                ssize_t sent = send(_pollfds[i].fd,
                                    resp.c_str() + _clients[_pollfds[i].fd]->getSendOffset(),
                                    resp.size() - _clients[_pollfds[i].fd]->getSendOffset(),
                                    0);

                if (sent < 0) {
                    Logger::error("Failed to send data to (fd: " + intToString(_pollfds[i].fd) + ")");
                    Logger::info("Closing connection (fd: " + intToString(_pollfds[i].fd) + ")");
                    delete _clients[_pollfds[i].fd];
                    _clients.erase(_pollfds[i].fd);
                    close(_pollfds[i].fd);
                    _pollfds[i] = _pollfds[_n_fds - 1];
                    _n_fds--;
                    i--;
                }
                else {
                    
                    _clients[_pollfds[i].fd]->setSendOffset(_clients[_pollfds[i].fd]->getSendOffset() + sent);

                    if (_clients[_pollfds[i].fd]->getSendOffset() >= resp.size()) {
                        _clients[_pollfds[i].fd]->clearResponseBuffer();
                        _clients[_pollfds[i].fd]->setState(READING);
                        _clients[_pollfds[i].fd]->setSendOffset(0);
                        _pollfds[i].events = POLLIN;
                    }
                }
            }
        }
    }

        // Prepare pollfd array based on _config (listeners, clients, CGI pipes)
        // Call poll()
        // For each ready fd, dispatch to appropriate handler based on event type
        // e.g., handleRead(fd), handleWrite(fd)
        
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
