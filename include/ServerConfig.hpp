#pragma once

#include <string>
#include <vector>
#include "ConfigNode.hpp"
#include "Location.hpp"

class ServerConfig
{
private:
  std::vector<ConfigNode> nodes_;
  std::string host_;
  int port_;
  int socket_fd_;

public:
  ServerConfig(const std::string &host = "localhost", int port = 80);

  void setHost(const std::string &host) ;
  const std::string &getHost() const;
  void setPort(int port) ;
  int getPort() const;
  void setSocketFD(int fd) ;
  int getSocketFD() const;

  std::vector<std::string> getServerNames(const ConfigNode& serverNode) const;
  void addServerNode(const ConfigNode &node);
  const std::vector<ConfigNode>& getNodes() const;

};
