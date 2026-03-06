#pragma once

#include <vector>
#include <map>
#include <set>
#include <poll.h>
#include "Config.hpp"
#include "Logger.hpp"

class Client;

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

        // Maps CGI pipe fds back to the client fd that owns them
        std::map<int, int> _cgiPipeToClient;

        void acceptNewConnection(int listenerFd);
        void handleClientRead(int index);
        void handleClientWrite(int index);
        void removeClient(int index);

        // CGI integration into event loop
        void registerCgiPipes(Client* client);
        void unregisterCgiPipes(Client* client);
        void handleCgiPipeRead(int pipeFd);
        void handleCgiPipeWrite(int pipeFd);
        void finalizeCgiResponse(Client* client);
        void checkCgiTimeouts();

        // Pollfd helpers
        void removePollFd(int fd);
        void setPollEvents(int fd, short events);
        void sendErrorAndFinish(Client* client, int statusCode, const std::string& msg,
                                const Location& location, bool keepAlive);
};

