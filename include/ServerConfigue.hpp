#pragma once

#include <string>
#include <vector>
#include "ConfigNode.hpp"
#include "Location.hpp"
#include <iostream>

using namespace std;

class ServerConfigue
{
private:
  ConfigNode node_;
  string host_;
  int port_;
  int socket_fd_;

public:
  ServerConfigue(const ConfigNode &node = ConfigNode(), const string &host = "localhost", int port = 80)
      : node_(node), host_(host), port_(port), socket_fd_(-1) {}
  void setNode(const ConfigNode &node) { node_ = node; }
  const ConfigNode &getNode() const { return node_; }
  void setHost(const string &host) { host_ = host; }
  const string &getHost() const { return host_; }
  void setPort(int port) { port_ = port; }
  int getPort() const { return port_; }
  void setSocketFD(int fd) { socket_fd_ = fd; }
  int getSocketFD() const { return socket_fd_; }
};
