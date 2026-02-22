#pragma once

#include "webserv.hpp"
#include "Config.hpp"
#include "EventLoop.hpp"

class Server {
public:
    Server(Config *config);
    ~Server();

    bool init();
    void run();
    void stop();

    static Server* _instance;
    static void handleSignal(int sig);

    bool    setupServerSockets();
    bool    setOptions();
    bool    bindSocket();

private:
    Config *_config;
    EventLoop _eventLoop;
    std::vector<int> _listenFds;
    bool _running;

    bool setupListeners();
    void cleanup();
};

