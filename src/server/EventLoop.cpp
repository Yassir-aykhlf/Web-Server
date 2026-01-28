/*
The heart of the non-blocking architecture.
It implements the Reactor pattern using poll().
It monitors all file descriptors (listeners, clients, CGI pipes)
and dispatches events (POLLIN/POLLOUT) to the appropriate read/write handlers.
*/

#include "EventLoop.hpp"

EventLoop::EventLoop() : _config(NULL), _running(false), _n_fds(1) {}

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
    int fd_socket = _config->getFdSocket();

    // Main event loop
    memset(_pollfds, 0, sizeof(_pollfds));
    _pollfds[0].fd = fd_socket;
    _pollfds[0].events = POLLIN;

    // Polling loop
    _config->getListenFds();
    while (_running) {
        int poll_count = poll(_pollfds, n_fds, -1);
        if (poll_count < 0) {
            Logger::error("Poll error");
            close(fd_socket);
            return false;
        }

        // Check for new connections
        if (_pollfds[0].revents & POLLIN) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(fd_socket, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                Logger::error("Failed to accept connection");
                continue;
            }
            Logger::info("Accepted connection from " + std::string(inet_ntoa(client_addr.sin_addr)) + ":" + intToString(ntohs(client_addr.sin_port)) + " (fd: " + intToString(client_fd) + ")");
            non_blocking(client_fd);
            _pollfds[_n_fds].fd = client_fd;
            _pollfds[_n_fds].events = POLLIN;
            _clients[client_fd] = new Client(client_fd);
            _n_fds++;
        }

        // Check existing connections for data
        for (int i = 1; i < _n_fds; i++) {
            if (_pollfds[i].revents & POLLIN) {
                char buffer[1024];
                ssize_t bytes_read = recv(_pollfds[i].fd, buffer, sizeof(buffer), 0);
                if (bytes_read <= 0) {
                    Logger::info("Closing connection (fd: " + intToString(_pollfds[i].fd) + ")");
                    delete _clients[_pollfds[i].fd];
                    _clients.erase(_pollfds[i].fd);
                    close(_pollfds[i].fd);
                    _pollfds[i] = _pollfds[_n_fds - 1];
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
                                    resp.c_str() + _clients[_pollfds[i].fd]->_sendOffset,
                                    resp.size() - _clients[_pollfds[i].fd]->_sendOffset,
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
                    
                    _clients[_pollfds[i].fd]->setSendOffset(_clients[_pollfds[i].fd]->_sendOffset + sent);

                    if (_clients[_pollfds[i].fd]->_sendOffset >= resp.size()) {
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
