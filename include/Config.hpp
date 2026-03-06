#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "ConfigNode.hpp"
#include "ConfigParser.hpp"
#include "ConfigExceptions.hpp"
#include "ConfigValidator.hpp"
#include "ServerConfig.hpp"
#include "ConfigRouter.hpp"
#include "URI.hpp"

class Config
{
private:
  ConfigNode ast_;
  std::string filename_;
  std::vector<ServerConfig> ServerConfigs_;

public:
  Config(const std::string filename);
  void load();

  bool isIPv4(const std::string &ip);
  void inetAddressStr(sockaddr *addr, socklen_t addrlen, std::string &host, std::string &port);
  std::string getIpByHost(const std::string &host);
  std::pair<std::string, int> parseListenArgument(const std::string &arg);
  std::vector<std::pair<std::string, int> > getAllListenInfo(const ConfigNode &serverNode);
  std::vector<std::string> getServerNames(const ConfigNode &serverNode);
  std::vector<ServerConfig>& getServerConfigs();
  void setServerConfigs();
};

