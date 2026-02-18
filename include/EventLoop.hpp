#ifndef EVENTLOOP_HPP
#define EVENTLOOP_HPP

#include <vector>
#include <map>
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
        // 
        const std::vector<struct pollfd> getListenFds() const;
        void addListenFd(struct pollfd pfd);
        int removeListenFd(int index);
        void cleanup();

    private:
        Config* _config;
        // Server* _server;
        bool _running;
        int _n_fds;
        std::vector<struct pollfd> _pollfds;
        std::map<int, Client*> _clients;
};

#endif
