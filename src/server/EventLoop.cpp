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
#include <csignal>
#include <sys/wait.h>
#include <cerrno>

EventLoop::EventLoop() : _config(NULL), _running(false), _n_fds(0), _nextClientBirthOrder(1) {}

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
    _clientBirthOrder.erase(fd);
    close(fd);
    _pollfds[index] = _pollfds.back();
    _pollfds.pop_back();
    _n_fds--;
}

void EventLoop::acceptNewConnection(int listenerFd)
{
    bool atCapacity = (activeClientCount() >= MAX_KEEP_ALIVE_CLIENTS);
    if (activeClientCount() >= MAX_KEEP_ALIVE_CLIENTS)
    {
        if (!evictOldestKeepAliveClient())
            Logger::warning("Client capacity reached and no evictable keep-alive client found");
    }

    ClientConnection conn = waitForClient(listenerFd);
    if (conn.fd < 0)
    {
        Logger::error("Failed to accept connection");
        return;
    }

    if (atCapacity && activeClientCount() >= MAX_KEEP_ALIVE_CLIENTS)
    {
        Logger::warning("Rejecting new connection at capacity (fd: " + intToString(conn.fd) + ")");
        close(conn.fd);
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
    _clientBirthOrder[conn.fd] = _nextClientBirthOrder++;
    _n_fds++;
}

size_t EventLoop::activeClientCount() const
{
    size_t count = 0;
    for (std::map<int, Client *>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (it->second != NULL)
            count++;
    }
    return count;
}

int EventLoop::findOldestEvictableClientPollIndex() const
{
    int bestIdleIndex = -1;
    unsigned long long bestIdleOrder = 0;

    for (int i = 0; i < _n_fds; i++)
    {
        int fd = _pollfds[i].fd;
        if (isListenerSocket(fd) || isCgiPipeFd(fd))
            continue;

        std::map<int, Client *>::const_iterator it = _clients.find(fd);
        if (it == _clients.end() || it->second == NULL)
            continue;

        std::map<int, unsigned long long>::const_iterator orderIt = _clientBirthOrder.find(fd);
        if (orderIt == _clientBirthOrder.end())
            continue;
        unsigned long long order = orderIt->second;

        Client *client = it->second;
        bool idleKeepAlive = (client->getState() == READING &&
                              client->getRequestBuffer().empty() &&
                              client->getResponseBuffer().empty());
        if (!idleKeepAlive)
            continue;

        if (bestIdleIndex == -1 || order < bestIdleOrder)
        {
            bestIdleIndex = i;
            bestIdleOrder = order;
        }
    }

    return bestIdleIndex;
}

bool EventLoop::evictOldestKeepAliveClient()
{
    int pollIndex = findOldestEvictableClientPollIndex();
    if (pollIndex < 0 || pollIndex >= _n_fds)
        return false;

    int fd = _pollfds[pollIndex].fd;
    Logger::warning("Evicting oldest keep-alive client (fd: " + intToString(fd) + ") due to capacity pressure");
    removeClient(pollIndex);
    return true;
}

void EventLoop::handleClientRead(int index)
{
    int fd = _pollfds[index].fd;
    Client *client = _clients[fd];

    // If we're in the middle of writing a response, skip reading new data
    // to avoid interrupting large response transmission

    bool connectionClosed = false;
    bool readError = false;
    std::string data = receiveData(fd, BUFFER_SIZE, connectionClosed, readError);
    if (readError)
    {
        Logger::warning("Closing client due to socket read error");
        removeClient(index);
        return;
    }
    if (connectionClosed)
    {
        if (client)
        {
            bool hasPendingResponse = !client->getResponseBuffer().empty() &&
                                      !isEntireResponseSent(client);
            bool cgiInProgress = (client->getState() == CGI_PROCESSING);

            if (cgiInProgress)
            {
                _pollfds[index].events = 0;
                return;
            }
            if (hasPendingResponse)
            {
                _pollfds[index].events = POLLOUT;
                return;
            }

            if (client->hasIncompleteRequest())
            {
                client->appendToResponseBuffer(
                    HttpResponse::makeError(STATUS_BAD_REQUEST, "Incomplete request body").build());
                client->setState(WRITING);
                _pollfds[index].events = POLLOUT;
                return;
            }

            if (!client->getRequestBuffer().empty())
            {
                size_t bufferSizeBefore = client->getResponseBuffer().length();
                client->buildResponse();
                size_t bufferSizeAfter = client->getResponseBuffer().length();

                if (client->getState() == CGI_PROCESSING)
                {
                    registerCgiPipes(client);
                    _pollfds[index].events = 0;
                    return;
                }
                if (bufferSizeAfter > bufferSizeBefore)
                {
                    client->setState(WRITING);
                    _pollfds[index].events = POLLOUT;
                    return;
                }
            }
        }
        removeClient(index);
        return;
    }
    if (data.empty())
        return;
    client->appendToRequestBuffer(data);
    size_t bufferSizeBefore = client->getResponseBuffer().length();
    client->buildResponse();
    size_t bufferSizeAfter = client->getResponseBuffer().length();

    if (client->getState() == CGI_PROCESSING)
    {
        registerCgiPipes(client);
        _pollfds[index].events = 0;
    }
    else if (bufferSizeAfter > bufferSizeBefore)
    {
        client->setState(WRITING);
        _pollfds[index].events = POLLOUT;
    }
}

void EventLoop::handleClientWrite(int index)
{
    int fd = _pollfds[index].fd;
    Client *client = _clients[fd];

    if (!client)
        return;

    if (client->getState() == CGI_PROCESSING)
    {
        _pollfds[index].events = 0;
        return;
    }

    const std::string &resp = client->getResponseBuffer();
    if (resp.empty())
    {
        client->setState(READING);
        _pollfds[index].events = POLLIN;
        return;
    }

    // Send data in a loop until we can't send anymore or response is complete.
    // For non-blocking sockets, send() might return -1 if the kernel buffer is full,
    // in which case we should retry. Since we can't check errno, we just keep trying
    // until send() fails consistently or we've sent the entire response.
    int sendAttempts = 0;
    int maxConsecutiveZeros = 2;
    int consecutiveZeros = 0;

    while (!isEntireResponseSent(client) && sendAttempts < 1000)
    {
        sendAttempts++;
        ssize_t sent = sendData(fd, resp, client->getSendOffset());

        if (sent > 0)
        {
            consecutiveZeros = 0;
            client->setSendOffset(client->getSendOffset() + sent);
            continue;
        }

        if (sent == 0)
        {
            consecutiveZeros++;
            if (consecutiveZeros >= maxConsecutiveZeros)
            {
                // Multiple send() calls returned 0, likely connection closed
                break;
            }
            continue;
        }

        // sent < 0: error. With non-blocking sockets, this could be EAGAIN.
        // We can't check errno, so we just return and wait for next POLLOUT.
        break;
    }

    if (isEntireResponseSent(client))
    {
        if (shouldCloseConnection(resp))
        {
            Logger::warning("Closing client due to Connection: close after full response");
            removeClient(index);
        }
        else
        {
            resetClientForNextRequest(client, index);
        }
    }
    else
    {
        // More data to send, keep in POLLOUT mode
        _pollfds[index].events = POLLOUT;
    }
}

bool EventLoop::isEntireResponseSent(Client *client) const
{
    return client->getSendOffset() >= client->getResponseBuffer().size();
}

bool EventLoop::shouldCloseConnection(const std::string &response) const
{
    size_t headerEnd = response.find("\r\n\r\n");
    std::string headerPart = (headerEnd == std::string::npos) ? response : response.substr(0, headerEnd);
    std::string lowerHeaders = toLower(headerPart);
    return lowerHeaders.find("connection: close") != std::string::npos;
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

bool EventLoop::readAllAvailableData(int pipeFd, std::string &outputBuffer, bool &readError)
{
    char buffer[BUFFER_SIZE];
    readError = false;
    ssize_t bytesRead = read(pipeFd, buffer, sizeof(buffer));
    if (bytesRead > 0)
    {
        outputBuffer.append(buffer, bytesRead);
        return false;
    }
    if (bytesRead == 0)
        return true;
    // bytesRead < 0: with non-blocking pipe, this means no data available or error.
    // We don't check errno; we just return and let caller handle it.
    return false;
}

void EventLoop::handleCgiPipeRead(int pipeFd)
{
    Client *client = findClientForCgiPipe(pipeFd);
    if (!client)
        return;
    CgiProcess &cgi = client->getCgiProcess();
    size_t oldSize = cgi.outputBuffer.size();
    bool readError = false;
    bool pipeEof = readAllAvailableData(pipeFd, cgi.outputBuffer, readError);
    if (readError)
    {
        bool keepAlive = cgi.keepAlive;
        Location location = cgi.location;
        unregisterCgiPipes(client);
        sendErrorAndFinish(client, STATUS_INTERNAL_SERVER_ERROR, "CGI pipe read error", location, keepAlive);
        return;
    }
    if (cgi.outputBuffer.size() > oldSize)
        cgi.startTime = time(NULL);
    if (pipeEof)
        finalizeCgiResponse(client);
}

void EventLoop::handleCgiPipeWrite(int pipeFd)
{
    Client *client = findClientForCgiPipe(pipeFd);
    if (!client)
        return;
    CgiProcess &cgi = client->getCgiProcess();

    if (cgi.bytesWritten >= cgi.bodyToWrite.length())
    {
        cgi.stdinDone = true;
        cgi.bytesWritten = 0;
        std::string().swap(cgi.bodyToWrite);
        _cgiPipeToClient.erase(pipeFd);
        removePollFd(pipeFd);
        cgi.pipeIn = -1;
        return;
    }

    size_t remaining = cgi.bodyToWrite.length() - cgi.bytesWritten;
    ssize_t written = write(pipeFd,
                            cgi.bodyToWrite.c_str() + cgi.bytesWritten,
                            remaining);

    if (written > 0)
    {
        cgi.bytesWritten += static_cast<size_t>(written);
        cgi.startTime = time(NULL);
        if (cgi.bytesWritten >= cgi.bodyToWrite.length())
        {
            cgi.stdinDone = true;
            cgi.bytesWritten = 0;
            std::string().swap(cgi.bodyToWrite);
            _cgiPipeToClient.erase(pipeFd);
            removePollFd(pipeFd);
            cgi.pipeIn = -1;
        }
        return;
    }

    if (written < 0)
    {
        // written == -1: with non-blocking pipe, this could be EAGAIN (buffer full)
        // or a real error. We don't check errno. Just return and let poll tell us
        // when to retry. If poll keeps telling us POLLOUT but write still fails,
        // we'll eventually hit the timeout and error out.
        return;
    }

    Logger::error("Failed to write to CGI pipe (fd: " + intToString(pipeFd) + ")");
    bool keepAlive = cgi.keepAlive;
    Location location = cgi.location;
    unregisterCgiPipes(client);
    if (written == 0)
        sendErrorAndFinish(client, STATUS_INTERNAL_SERVER_ERROR, "CGI pipe closed unexpectedly", location, keepAlive);
    else
        sendErrorAndFinish(client, STATUS_INTERNAL_SERVER_ERROR, "CGI pipe write error", location, keepAlive);
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
    std::string output;
    output.swap(cgi.outputBuffer);
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
    size_t bodyStart = 0;
    CgiHandler::parseCgiHeadersOnly(output, response, bodyStart);
    size_t bodyLength = 0;
    if (bodyStart < output.length())
        bodyLength = output.length() - bodyStart;
    response.setContentLength(bodyLength);
    response.setConnection(keepAlive);

    client->clearResponseBuffer();
    client->appendToResponseBuffer(response.build());
    if (bodyLength > 0)
        client->appendToResponseBuffer(output.data() + bodyStart, bodyLength);

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
    bool shouldAttemptWrite = false;

    if (_pollfds[index].revents & (POLLERR | POLLNVAL))
    {
        int fd = _pollfds[index].fd;
        Client *client = _clients[fd];
        if (!client)
        {
            Logger::warning("Closing client due to POLLERR/POLLNVAL with null client");
            removeClient(index);
            return;
        }
        bool hasPendingResponse = !client->getResponseBuffer().empty() &&
                                  !isEntireResponseSent(client);
        bool cgiInProgress = (client->getState() == CGI_PROCESSING);
        bool isWriting = (client->getState() == WRITING);
        if (!hasPendingResponse && !cgiInProgress && !isWriting)
        {
            Logger::warning("Closing client due to POLLERR/POLLNVAL without pending response/CGI");
            removeClient(index);
            return;
        }
        // If we're WRITING, try to continue sending despite POLLERR
        // The socket error might be recoverable or we might still be able to send
        if (isWriting)
            shouldAttemptWrite = true;
    }
    if ((_pollfds[index].revents & POLLHUP) && !(_pollfds[index].revents & POLLIN))
    {
        int fd = _pollfds[index].fd;
        Client *client = _clients[fd];
        if (!client)
        {
            Logger::warning("Closing client due to POLLHUP with null client");
            removeClient(index);
            return;
        }
        bool hasPendingResponse = !client->getResponseBuffer().empty() &&
                                  !isEntireResponseSent(client);
        bool cgiInProgress = (client->getState() == CGI_PROCESSING);
        bool isWriting = (client->getState() == WRITING);
        if (!hasPendingResponse && !cgiInProgress && !isWriting)
        {
            Logger::warning("Closing client due to POLLHUP without pending response/CGI");
            removeClient(index);
            return;
        }
        // If we're WRITING, try to continue sending despite POLLHUP
        if (isWriting)
            shouldAttemptWrite = true;
    }

    if (shouldAttemptWrite || (_pollfds[index].revents & POLLOUT))
    {
        handleClientWrite(index);
        if (index >= _n_fds)
            return;
    }
    if (_pollfds[index].revents & POLLIN)
    {
        handleClientRead(index);
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
    cleanup();
}

void EventLoop::stop()
{
    _running = false;
}

void EventLoop::cleanup()
{
    for (int i = _n_fds - 1; i >= 0; --i)
    {
        int fd = _pollfds[i].fd;
        if (isListenerSocket(fd))
        {
            close(fd);
            _listenSockets.erase(fd);
            _clients.erase(fd);
            _pollfds[i] = _pollfds.back();
            _pollfds.pop_back();
            _n_fds--;
            continue;
        }
        if (_clients.find(fd) != _clients.end() && _clients[fd])
        {
            removeClient(i);
            continue;
        }
        close(fd);
        _clients.erase(fd);
        _cgiPipeToClient.erase(fd);
        _pollfds[i] = _pollfds.back();
        _pollfds.pop_back();
        _n_fds--;
    }
    _pollfds.clear();
    _clients.clear();
    _cgiPipeToClient.clear();
    _listenSockets.clear();
    _clientBirthOrder.clear();
    _nextClientBirthOrder = 1;
}
