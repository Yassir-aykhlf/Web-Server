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
  vector<ConfigNode> nodes_;
  string host_;
  int port_;
  int socket_fd_;

public:
  ServerConfigue(const string &host = "localhost", int port = 80);

  void setHost(const string &host) ;
  const string &getHost() const;
  void setPort(int port) ;
  int getPort() const;
  void setSocketFD(int fd) ;
  int getSocketFD() const;

  vector<string> getServerNames(const ConfigNode& serverNode) const;
  void addServerNode(const ConfigNode &node);
  const vector<ConfigNode>& getNodes() const;

};
