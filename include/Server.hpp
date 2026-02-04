// #ifndef SERVER_HPP
// #define SERVER_HPP


// #include "webserv.hpp"
// #include "Config.hpp"

#include "webserv.hpp"
#include "Config.hpp"
#include "EventLoop.hpp"

// class Server {
// public:
//     Server();
//     ~Server();

//     bool init(const std::string& configFile);
//     void run();
//     void stop();


//     static Server* _instance;
//     static void handleSignal(int sig);



    bool    setupServerSockets();
    bool    setOptions();
    bool    bindSocket();


// private:
//     Config _config;
//     // EventLoop _eventLoop;
//     // std::vector<int> _listenFds;
//     bool _running;

//     bool setupListeners();
//     void cleanup();
// };

// #endif