#include "EventLoop.hpp"
#include "Client.hpp"
#include "CgiHandler.hpp"
#include "RequestHandler.hpp"
#include "webserv.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <csignal>
#include <sys/wait.h>

EventLoop::EventLoop() : _config(NULL), _running(false), _n_fds(0) {}

EventLoop::~EventLoop() {}

void EventLoop::setConfig(Config* config) {
    _config = config;
}

void EventLoop::removeClient(int index) {
    int fd = _pollfds[index].fd;
    Logger::info("Closing connection (fd: " + intToString(fd) + ")");

    // Clean up any active CGI for this client
    if (_clients.count(fd) && _clients[fd]) {
        unregisterCgiPipes(_clients[fd]);
        delete _clients[fd];
    }
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

    if (_clients[fd]->isRequestComplete()) {
        _clients[fd]->buildResponse();

        // Check if buildResponse started an async CGI
        if (_clients[fd]->getState() == CGI_PROCESSING) {
            // Register CGI pipe fds in the main poll loop
            registerCgiPipes(_clients[fd]);
            // Stop polling the client socket for now (neither read nor write)
            _pollfds[index].events = 0;
        } else {
            _clients[fd]->setState(WRITING);
            _pollfds[index].events = POLLOUT;
        }
    }
}

void EventLoop::handleClientWrite(int index) {
    int fd = _pollfds[index].fd;
    Client* client = _clients[fd];
    const std::string& resp = client->getResponseBuffer();

    if (resp.empty()) {
        Logger::error("Empty response buffer for fd: " + intToString(fd));
        removeClient(index);
        return;
    }

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

// ──────────────────────────────────────────────
// Pollfd helpers
// ──────────────────────────────────────────────

void EventLoop::removePollFd(int fd) {
    for (int i = 0; i < _n_fds; i++) {
        if (_pollfds[i].fd == fd) {
            close(fd);
            _pollfds[i] = _pollfds.back();
            _pollfds.pop_back();
            _n_fds--;
            return;
        }
    }
}

void EventLoop::setPollEvents(int fd, short events) {
    for (int i = 0; i < _n_fds; i++) {
        if (_pollfds[i].fd == fd) {
            _pollfds[i].events = events;
            return;
        }
    }
}

void EventLoop::sendErrorAndFinish(Client* client, int statusCode,
                                    const std::string& msg,
                                    const Location& location, bool keepAlive) {
    HttpResponse response = HttpResponse::makeError(statusCode, msg);
    response = RequestHandler::applyCustomErrorPage(response, location);
    response.setConnection(keepAlive);
    client->clearResponseBuffer();
    client->appendToResponseBuffer(response.build());
    client->setSendOffset(0);
    client->setState(WRITING);
    setPollEvents(client->getFd(), POLLOUT);
}

// ──────────────────────────────────────────────
// CGI pipe management — integrate into main poll
// ──────────────────────────────────────────────

void EventLoop::registerCgiPipes(Client* client) {
    CgiProcess& cgi = client->getCgiProcess();

    // Register the CGI stdout pipe for reading
    if (cgi.pipeOut >= 0) {
        struct pollfd pfd;
        pfd.fd = cgi.pipeOut;
        pfd.events = POLLIN;
        pfd.revents = 0;
        _pollfds.push_back(pfd);
        _cgiPipeToClient[cgi.pipeOut] = client->getFd();
        _n_fds++;
    }

    // Register the CGI stdin pipe for writing (if body needs to be sent)
    if (cgi.pipeIn >= 0 && !cgi.stdinDone) {
        struct pollfd pfd;
        pfd.fd = cgi.pipeIn;
        pfd.events = POLLOUT;
        pfd.revents = 0;
        _pollfds.push_back(pfd);
        _cgiPipeToClient[cgi.pipeIn] = client->getFd();
        _n_fds++;
    }
}

void EventLoop::unregisterCgiPipes(Client* client) {
    CgiProcess& cgi = client->getCgiProcess();

    if (cgi.isActive()) {
        kill(cgi.pid, SIGKILL);
        waitpid(cgi.pid, NULL, 0);
    }

    if (cgi.pipeOut >= 0) {
        _cgiPipeToClient.erase(cgi.pipeOut);
        removePollFd(cgi.pipeOut);
        cgi.pipeOut = -1;
    }

    if (cgi.pipeIn >= 0) {
        _cgiPipeToClient.erase(cgi.pipeIn);
        removePollFd(cgi.pipeIn);
        cgi.pipeIn = -1;
    }

    cgi.reset();
}

void EventLoop::handleCgiPipeRead(int pipeFd) {
    // Find which client owns this pipe
    if (_cgiPipeToClient.find(pipeFd) == _cgiPipeToClient.end())
        return;
    int clientFd = _cgiPipeToClient[pipeFd];
    if (_clients.find(clientFd) == _clients.end() || !_clients[clientFd])
        return;

    Client* client = _clients[clientFd];
    CgiProcess& cgi = client->getCgiProcess();
    char buffer[BUFFER_SIZE];
    bool pipeEof = false;

    while (true) {
        ssize_t bytesRead = read(pipeFd, buffer, sizeof(buffer));
        if (bytesRead > 0) {
            cgi.outputBuffer.append(buffer, bytesRead);
            continue;
        }
        if (bytesRead == 0) {
            pipeEof = true;
            break;
        }
        if (bytesRead < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        }
        if (bytesRead < 0 && errno != EINTR) {
            pipeEof = true;
        }
        break;
    }

    if (pipeEof) {
        // CGI stdout is done — finalize the response
        finalizeCgiResponse(client);
    }
}

void EventLoop::handleCgiPipeWrite(int pipeFd) {
    if (_cgiPipeToClient.find(pipeFd) == _cgiPipeToClient.end())
        return;
    int clientFd = _cgiPipeToClient[pipeFd];
    if (_clients.find(clientFd) == _clients.end() || !_clients[clientFd])
        return;

    Client* client = _clients[clientFd];
    CgiProcess& cgi = client->getCgiProcess();

    ssize_t written = write(pipeFd,
        cgi.bodyToWrite.c_str() + cgi.bytesWritten,
        cgi.bodyToWrite.length() - cgi.bytesWritten);

    if (written > 0) {
        cgi.bytesWritten += static_cast<size_t>(written);
        if (cgi.bytesWritten >= cgi.bodyToWrite.length()) {
            cgi.stdinDone = true;
            _cgiPipeToClient.erase(pipeFd);
            removePollFd(pipeFd);
            cgi.pipeIn = -1;
        }
    } else if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        Logger::error("Failed to write to CGI stdin pipe");
        cgi.stdinDone = true;
        _cgiPipeToClient.erase(pipeFd);
        removePollFd(pipeFd);
        cgi.pipeIn = -1;
    }
}

void EventLoop::finalizeCgiResponse(Client* client) {
    CgiProcess& cgi = client->getCgiProcess();

    int childStatus = 0;
    pid_t result = waitpid(cgi.pid, &childStatus, WNOHANG);
    if (result == 0) {
        kill(cgi.pid, SIGKILL);
        waitpid(cgi.pid, &childStatus, 0);
    }

    pid_t savedPid = cgi.pid;
    cgi.pid = -1;

    std::string output = cgi.outputBuffer;
    bool keepAlive = cgi.keepAlive;
    Location location = cgi.location;

    unregisterCgiPipes(client);

    if (WIFEXITED(childStatus) && WEXITSTATUS(childStatus) != 0 && output.empty()) {
        Logger::error("CGI script exited with error (pid: " + intToString(savedPid) + ")");
        sendErrorAndFinish(client, STATUS_INTERNAL_SERVER_ERROR, "CGI script error", location, keepAlive);
        return;
    }

    HttpResponse response;
    CgiHandler::parseCgiOutput(output, response);
    response.setConnection(keepAlive);
    client->clearResponseBuffer();
    client->appendToResponseBuffer(response.build());
    client->setSendOffset(0);
    client->setState(WRITING);
    setPollEvents(client->getFd(), POLLOUT);
}

void EventLoop::checkCgiTimeouts() {
    time_t now = time(NULL);

    std::vector<int> timedOut;
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        Client* client = it->second;
        if (!client || client->getState() != CGI_PROCESSING)
            continue;
        CgiProcess& cgi = client->getCgiProcess();
        if (!cgi.isActive())
            continue;
        if (static_cast<int>(now - cgi.startTime) >= cgi.timeoutSec)
            timedOut.push_back(it->first);
    }

    for (size_t i = 0; i < timedOut.size(); i++) {
        if (_clients.find(timedOut[i]) == _clients.end())
            continue;
        Client* client = _clients[timedOut[i]];
        if (!client)
            continue;

        CgiProcess& cgi = client->getCgiProcess();
        Logger::error("CGI timeout (pid: " + intToString(cgi.pid) + ")");

        bool keepAlive = cgi.keepAlive;
        Location location = cgi.location;
        unregisterCgiPipes(client);

        sendErrorAndFinish(client, STATUS_GATEWAY_TIMEOUT, "CGI script timed out", location, keepAlive);
    }
}

// ──────────────────────────────────────
// Main event loop
// ──────────────────────────────────────

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

    // Main event loop — poll timeout of 200ms to check CGI timeouts
    while (_running) {
        int poll_count = poll(&_pollfds[0], _pollfds.size(), 200);
        if (poll_count < 0) {
            if (errno == EINTR)
                continue;
            Logger::error("Poll error");
            cleanup();
            return;
        }

        // Check CGI timeouts on every iteration
        checkCgiTimeouts();

        for (int i = 0; i < _n_fds; i++) {
            if (_pollfds[i].revents == 0)
                continue;

            int fd = _pollfds[i].fd;

            // Listener sockets
            bool isListener = (_listenSockets.count(fd) > 0);
            if (isListener) {
                if (_pollfds[i].revents & POLLIN)
                    acceptNewConnection(fd);
                continue;
            }

            // CGI pipe fds
            bool isCgiPipe = (_cgiPipeToClient.count(fd) > 0);
            if (isCgiPipe) {
                if (_pollfds[i].revents & (POLLIN | POLLHUP)) {
                    handleCgiPipeRead(fd);
                    if (i >= _n_fds) break;
                }
                if (i < _n_fds && (_pollfds[i].revents & POLLOUT)) {
                    handleCgiPipeWrite(fd);
                    if (i >= _n_fds) break;
                }
                continue;
            }

            // Regular client sockets
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

void EventLoop::cleanup() {
    for (size_t i = 0; i < _pollfds.size(); ++i) {
        close(_pollfds[i].fd);
    }
    _pollfds.clear();
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        delete it->second;
    }
    _clients.clear();
    _cgiPipeToClient.clear();
}

void EventLoop::stop() {
    _running = false;
}
