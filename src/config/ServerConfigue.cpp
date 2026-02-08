#include "ServerConfigue.hpp"



ServerConfigue::ServerConfigue(const ConfigNode &node, const string &host, int port)
: node_(node), host_(host), port_(port), socket_fd_(-1) {}
void   ServerConfigue::setNode(const ConfigNode &node) { node_ = node; }
const ConfigNode &  ServerConfigue::getNode() const { return node_; }
void   ServerConfigue::setHost(const string &host) { host_ = host; }
const string &  ServerConfigue::getHost() const { return host_; }
void   ServerConfigue::setPort(int port) { port_ = port; }
int   ServerConfigue::getPort() const { return port_; }
void   ServerConfigue::setSocketFD(int fd) { socket_fd_ = fd; }
int   ServerConfigue::getSocketFD() const { return socket_fd_; }