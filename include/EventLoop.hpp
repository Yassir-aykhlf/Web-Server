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
        const std::vector<struct pollfd> getListenFds() const;
        void addListenFd(struct pollfd pfd);
        int removeListenFd(int index);
        void cleanup();

    private:
        Config* _config;
        bool _running;
        int _n_fds;
        std::vector<struct pollfd> _pollfds;
        std::map<int, Client*> _clients;
        std::set<int> _listenSockets;

        void acceptNewConnection(int listenerFd);
        void handleClientRead(int index);
        void handleClientWrite(int index);
        void removeClient(int index);
};

