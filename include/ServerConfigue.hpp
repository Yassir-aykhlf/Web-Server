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
  ServerConfigue(const ConfigNode &node = ConfigNode(), const string &host = "localhost", int port = 80);
  void setNode(const ConfigNode &node);
  const ConfigNode &getNode() const ;
  void setHost(const string &host) ;
  const string &getHost() const;
  void setPort(int port) ;
  int getPort() const;
  void setSocketFD(int fd) ;
  int getSocketFD() const;
};
