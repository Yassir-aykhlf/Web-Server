// #ifndef SERVER_HPP
// #define SERVER_HPP

// #include "webserv.hpp"
// #include "Config.hpp"

// class Server {
// public:
//     Server();
//     ~Server();

//     bool init(const std::string& configFile);
//     void run();
//     void stop();

//     static Server* _instance;
//     static void handleSignal(int sig);

// private:
//     Config _config;
//     // EventLoop _eventLoop;
//     // std::vector<int> _listenFds;
//     bool _running;

//     bool setupListeners();
//     void cleanup();
// };

// #endif