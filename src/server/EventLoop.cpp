#include "EventLoop.hpp"
#include "Client.hpp"
#include "CgiHandler.hpp"
#include "RequestHandler.hpp"
#include "webserv.hpp"
#include "socket_utils.h"
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

void EventLoop::setConfig(Config *config)
{
    _config = config;
}

void EventLoop::removeClient(int index)
{
    int fd = _pollfds[index].fd;
    Logger::info("Closing connection (fd: " + intToString(fd) + ")");
    if (_clients.count(fd) && _clients[fd])
    {
        unregisterCgiPipes(_clients[fd]);
        delete _clients[fd];
    }
    _clients.erase(fd);
    close(fd);
    _pollfds[index] = _pollfds.back();
    _pollfds.pop_back();
    _n_fds--;
}

void EventLoop::acceptNewConnection(int listenerFd)
{
    ClientConnection conn = waitForClient(listenerFd);
    if (conn.fd < 0)
    {
        Logger::error("Failed to accept connection");
        return;
    }
    Logger::info("Accepted connection from " + conn.ipAddress + ":" + intToString(conn.port) + " (fd: " + intToString(conn.fd) + ")");
    non_blocking(conn.fd);
    struct pollfd client_pfd;
    client_pfd.fd = conn.fd;
    client_pfd.events = POLLIN;
    client_pfd.revents = 0;
    _pollfds.push_back(client_pfd);
    _clients[conn.fd] = new Client(conn.fd, _config, listenerFd);
    _n_fds++;
}

void EventLoop::handleClientRead(int index)
{
    int fd = _pollfds[index].fd;
    bool connectionClosed = false;
    std::string data = receiveData(fd, 1024, connectionClosed);
    if (connectionClosed)
    {
        removeClient(index);
        return;
    }
    _clients[fd]->appendToRequestBuffer(data);
    if (_clients[fd]->isRequestComplete())
    {
        size_t bufferSizeBefore = _clients[fd]->getResponseBuffer().length();
        _clients[fd]->buildResponse();
        size_t bufferSizeAfter = _clients[fd]->getResponseBuffer().length();

        if (_clients[fd]->getState() == CGI_PROCESSING)
        {
            registerCgiPipes(_clients[fd]);
            _pollfds[index].events = 0;
        }
        else if (bufferSizeAfter > bufferSizeBefore)
        {
            // Response was actually built, wait to send it
            _clients[fd]->setState(WRITING);
            _pollfds[index].events = POLLOUT;
        }
        // else: parser was incomplete, stay in READING and wait for more data
    }
}

void EventLoop::handleClientWrite(int index)
{
    int fd = _pollfds[index].fd;
    Client *client = _clients[fd];
    const std::string &resp = client->getResponseBuffer();
    if (resp.empty())
    {
        // Empty response buffer - socket is writable but we have nothing to send yet
        // This can happen on a freshly accepted connection before the request is parsed
        // Just ignore this event and wait for data to arrive
        return;
    }

    // If response is already fully sent, skip send and handle connection state
    if (!isEntireResponseSent(client))
    {
        ssize_t sent = sendData(fd, resp, client->getSendOffset());
        if (sent <= 0)
        {
            Logger::error("Failed to send data to (fd: " + intToString(fd) + ")");
            removeClient(index);
            return;
        }
        client->setSendOffset(client->getSendOffset() + sent);
    }

    if (isEntireResponseSent(client))
    {
        if (shouldCloseConnection(resp))
        {
            removeClient(index);
        }
        else
        {
            resetClientForNextRequest(client, index);
        }
    }
}

bool EventLoop::isEntireResponseSent(Client *client) const
{
    return client->getSendOffset() >= client->getResponseBuffer().size();
}

bool EventLoop::shouldCloseConnection(const std::string &response) const
{
    return response.find("Connection: close") != std::string::npos;
}

void EventLoop::resetClientForNextRequest(Client *client, int pollIndex)
{
    client->clearResponseBuffer();
    client->clearRequestBuffer();
    client->setState(READING);
    client->setSendOffset(0);
    _pollfds[pollIndex].events = POLLIN;
}

void EventLoop::removePollFd(int fd)
{
    for (int i = 0; i < _n_fds; i++)
    {
        if (_pollfds[i].fd == fd)
        {
            close(fd);
            _pollfds[i] = _pollfds.back();
            _pollfds.pop_back();
            _n_fds--;
            return;
        }
    }
}

void EventLoop::setPollEvents(int fd, short events)
{
    for (int i = 0; i < _n_fds; i++)
    {
        if (_pollfds[i].fd == fd)
        {
            _pollfds[i].events = events;
            return;
        }
    }
}

void EventLoop::sendErrorAndFinish(Client *client, int statusCode,
                                   const std::string &msg,
                                   const Location &location, bool keepAlive)
{
    HttpResponse response = HttpResponse::makeError(statusCode, msg);
    response = RequestHandler::applyCustomErrorPage(response, location);
    response.setConnection(keepAlive);
    client->clearResponseBuffer();
    client->appendToResponseBuffer(response.build());
    client->setSendOffset(0);
    client->setState(WRITING);
    setPollEvents(client->getFd(), POLLOUT);
}

void EventLoop::registerCgiPipes(Client *client)
{
    CgiProcess &cgi = client->getCgiProcess();
    if (cgi.pipeOut >= 0)
    {
        struct pollfd pfd;
        pfd.fd = cgi.pipeOut;
        pfd.events = POLLIN;
        pfd.revents = 0;
        _pollfds.push_back(pfd);
        _cgiPipeToClient[cgi.pipeOut] = client->getFd();
        _n_fds++;
    }
    if (cgi.pipeIn >= 0 && !cgi.stdinDone)
    {
        struct pollfd pfd;
        pfd.fd = cgi.pipeIn;
        pfd.events = POLLOUT;
        pfd.revents = 0;
        _pollfds.push_back(pfd);
        _cgiPipeToClient[cgi.pipeIn] = client->getFd();
        _n_fds++;
    }
}

void EventLoop::unregisterCgiPipes(Client *client)
{
    CgiProcess &cgi = client->getCgiProcess();
    if (cgi.isActive())
    {
        kill(cgi.pid, SIGKILL);
        waitpid(cgi.pid, NULL, 0);
    }
    if (cgi.pipeOut >= 0)
    {
        _cgiPipeToClient.erase(cgi.pipeOut);
        removePollFd(cgi.pipeOut);
        cgi.pipeOut = -1;
    }
    if (cgi.pipeIn >= 0)
    {
        _cgiPipeToClient.erase(cgi.pipeIn);
        removePollFd(cgi.pipeIn);
        cgi.pipeIn = -1;
    }
    cgi.reset();
}

Client *EventLoop::findClientForCgiPipe(int pipeFd)
{
    if (_cgiPipeToClient.find(pipeFd) == _cgiPipeToClient.end())
        return NULL;
    int clientFd = _cgiPipeToClient[pipeFd];
    if (_clients.find(clientFd) == _clients.end() || !_clients[clientFd])
        return NULL;
    return _clients[clientFd];
}

bool EventLoop::readAllAvailableData(int pipeFd, std::string &outputBuffer)
{
    char buffer[BUFFER_SIZE];
    bool pipeEof = false;
    while (true)
    {
        ssize_t bytesRead = read(pipeFd, buffer, sizeof(buffer));
        if (bytesRead > 0)
        {
            outputBuffer.append(buffer, bytesRead);
            continue;
        }
        if (bytesRead == 0)
        {
            pipeEof = true;
            break;
        }
        break;
    }
    return pipeEof;
}

void EventLoop::handleCgiPipeRead(int pipeFd)
{
    Client *client = findClientForCgiPipe(pipeFd);
    if (!client)
        return;
    CgiProcess &cgi = client->getCgiProcess();
    bool pipeEof = readAllAvailableData(pipeFd, cgi.outputBuffer);
    if (pipeEof)
        finalizeCgiResponse(client);
}

void EventLoop::handleCgiPipeWrite(int pipeFd)
{
    Client *client = findClientForCgiPipe(pipeFd);
    if (!client)
        return;
    CgiProcess &cgi = client->getCgiProcess();
    ssize_t written = write(pipeFd,
                            cgi.bodyToWrite.c_str() + cgi.bytesWritten,
                            cgi.bodyToWrite.length() - cgi.bytesWritten);
    if (written > 0)
    {
        cgi.bytesWritten += static_cast<size_t>(written);
        if (cgi.bytesWritten >= cgi.bodyToWrite.length())
        {
            cgi.stdinDone = true;
            _cgiPipeToClient.erase(pipeFd);
            removePollFd(pipeFd);
            cgi.pipeIn = -1;
        }
    }
    else if (written < 0)
    {
        Logger::error("Failed to write to CGI stdin pipe");
        cgi.stdinDone = true;
        _cgiPipeToClient.erase(pipeFd);
        removePollFd(pipeFd);
        cgi.pipeIn = -1;
    }
}

int EventLoop::reapChildProcess(pid_t pid)
{
    int childStatus = 0;
    pid_t result = waitpid(pid, &childStatus, WNOHANG);
    if (result == 0)
    {
        kill(pid, SIGKILL);
        waitpid(pid, &childStatus, 0);
    }
    return childStatus;
}

bool EventLoop::cgiExitedWithError(int childStatus, const std::string &output)
{
    return WIFEXITED(childStatus) && WEXITSTATUS(childStatus) != 0 && output.empty();
}

void EventLoop::finalizeCgiResponse(Client *client)
{
    CgiProcess &cgi = client->getCgiProcess();
    pid_t savedPid = cgi.pid;
    int childStatus = reapChildProcess(cgi.pid);
    cgi.pid = -1;
    std::string output = cgi.outputBuffer;
    bool keepAlive = cgi.keepAlive;
    Location location = cgi.location;
    unregisterCgiPipes(client);
    if (cgiExitedWithError(childStatus, output))
    {
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

bool EventLoop::hasCgiTimedOut(const CgiProcess &cgi, time_t now) const
{
    return static_cast<int>(now - cgi.startTime) >= cgi.timeoutSec;
}

void EventLoop::checkCgiTimeouts()
{
    time_t now = time(NULL);
    std::vector<int> timedOut;
    for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        Client *client = it->second;
        if (!client || client->getState() != CGI_PROCESSING)
            continue;
        CgiProcess &cgi = client->getCgiProcess();
        if (!cgi.isActive())
            continue;
        if (hasCgiTimedOut(cgi, now))
            timedOut.push_back(it->first);
    }
    for (size_t i = 0; i < timedOut.size(); i++)
    {
        if (_clients.find(timedOut[i]) == _clients.end())
            continue;
        Client *client = _clients[timedOut[i]];
        if (!client)
            continue;
        CgiProcess &cgi = client->getCgiProcess();
        Logger::error("CGI timeout (pid: " + intToString(cgi.pid) + ")");

        bool keepAlive = cgi.keepAlive;
        Location location = cgi.location;
        unregisterCgiPipes(client);

        sendErrorAndFinish(client, STATUS_GATEWAY_TIMEOUT, "CGI script timed out", location, keepAlive);
    }
}

bool EventLoop::registerListenerSockets()
{
    std::vector<ServerConfig> &serverConfig = _config->getServerConfigs();
    for (size_t i = 0; i < serverConfig.size(); ++i)
    {
        int fd_socket = serverConfig[i].getSocketFD();
        if (fd_socket < 0)
        {
            Logger::error("Invalid socket fd for server on " + serverConfig[i].getHost() + ":" + intToString(serverConfig[i].getPort()));
            return false;
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
    return true;
}

bool EventLoop::isListenerSocket(int fd) const
{
    return _listenSockets.count(fd) > 0;
}

bool EventLoop::isCgiPipeFd(int fd) const
{
    return _cgiPipeToClient.count(fd) > 0;
}

void EventLoop::dispatchListenerEvent(int index)
{
    if (_pollfds[index].revents & POLLIN)
        acceptNewConnection(_pollfds[index].fd);
}

void EventLoop::dispatchCgiPipeEvent(int index)
{
    int fd = _pollfds[index].fd;
    if (_pollfds[index].revents & (POLLIN | POLLHUP))
    {
        handleCgiPipeRead(fd);
        if (index >= _n_fds)
            return;
    }
    if (index < _n_fds && (_pollfds[index].revents & POLLOUT))
    {
        handleCgiPipeWrite(fd);
    }
}

void EventLoop::dispatchClientEvent(int index)
{
    if (_pollfds[index].revents & POLLIN)
    {
        handleClientRead(index);
        if (index >= _n_fds)
            return;
        if (_pollfds[index].revents & POLLOUT)
            return;
    }
    if (_pollfds[index].revents & POLLOUT)
    {
        handleClientWrite(index);
    }
}

void EventLoop::run()
{
    if (!_config)
    {
        Logger::error("EventLoop: No configuration set");
        return;
    }
    _running = true;
    Logger::info("EventLoop started");
    if (!registerListenerSockets())
        return;
    while (_running)
    {
        int poll_count = poll(&_pollfds[0], _pollfds.size(), POLL_TIMEOUT_MS);
        if (poll_count < 0)
        {
            Logger::error("Poll error");
            cleanup();
            return;
        }
        checkCgiTimeouts();
        for (int i = 0; i < _n_fds; i++)
        {
            if (_pollfds[i].revents == 0)
                continue;
            int fd = _pollfds[i].fd;
            if (isListenerSocket(fd))
            {
                dispatchListenerEvent(i);
                continue;
            }
            if (isCgiPipeFd(fd))
            {
                dispatchCgiPipeEvent(i);
                if (i >= _n_fds)
                    break;
                continue;
            }
            dispatchClientEvent(i);
            if (i >= _n_fds)
                break;
        }
    }
    Logger::info("EventLoop stopped");
}

void EventLoop::cleanup()
{
    for (size_t i = 0; i < _pollfds.size(); ++i)
    {
        close(_pollfds[i].fd);
    }
    _pollfds.clear();
    for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        delete it->second;
    }
    _clients.clear();
    _cgiPipeToClient.clear();
}

void EventLoop::stop()
{
    _running = false;
}
