#pragma once

#include <vector>
#include <map>
#include <set>
#include <poll.h>
#include "Config.hpp"
#include "Logger.hpp"

class Client;
struct CgiProcess;

class EventLoop {
    public:
        EventLoop();
        ~EventLoop();
        void setConfig(Config* config);
        void run();
        void stop();
        void cleanup();

    private:
        Config* _config;
        bool _running;
        int _n_fds;
        std::vector<struct pollfd> _pollfds;
        std::map<int, Client*> _clients;
        std::set<int> _listenSockets;

        std::map<int, int> _cgiPipeToClient;

        void acceptNewConnection(int listenerFd);
        void handleClientRead(int index);
        void handleClientWrite(int index);
        void removeClient(int index);

        bool isEntireResponseSent(Client* client) const;
        bool shouldCloseConnection(const std::string& response) const;
        void resetClientForNextRequest(Client* client, int pollIndex);

        void registerCgiPipes(Client* client);
        void unregisterCgiPipes(Client* client);
        void handleCgiPipeRead(int pipeFd);
        void handleCgiPipeWrite(int pipeFd);
        void finalizeCgiResponse(Client* client);
        void checkCgiTimeouts();
        Client* findClientForCgiPipe(int pipeFd);
        bool readAllAvailableData(int pipeFd, std::string& outputBuffer);
        int reapChildProcess(pid_t pid);
        bool cgiExitedWithError(int childStatus, const std::string& output);
        bool hasCgiTimedOut(const CgiProcess& cgi, time_t now) const;

        bool registerListenerSockets();
        bool isListenerSocket(int fd) const;
        bool isCgiPipeFd(int fd) const;
        void dispatchListenerEvent(int index);
        void dispatchCgiPipeEvent(int index);
        void dispatchClientEvent(int index);

        void removePollFd(int fd);
        void setPollEvents(int fd, short events);
        void sendErrorAndFinish(Client* client, int statusCode, const std::string& msg,
                                const Location& location, bool keepAlive);
};

