#ifndef CONFIG_HPP
#define CONFIG_HPP

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
#include "ServerConfigue.hpp"
#include "ConfigRouter.hpp"
#include "URI.hpp"

using namespace std;

class Config
{
private:
  ConfigNode ast_;
  string filename_;
  vector<ServerConfigue> ServerConfigues_;

public:
  Config(const string filename);
  void load();

  const ConfigNode &getAST() { return ast_; };

  bool isIPv4(const string &ip);
  void inetAddressStr(sockaddr *addr, socklen_t addrlen, string &host, string &port);
  string getIpByHost(const string &host) ;
  // Parse a listen argument and return the corresponding (ip, port) pair
  pair<string, int> parseListenArgument(const string &arg) ;
  vector<pair<string, int> > getAllListenInfo(const ConfigNode &serverNode) ;
  vector<string> getServerNames(const ConfigNode &serverNode) ;
  vector<ServerConfigue>& getServerConfigues();
  void setServerConfigues();

  // Debugging
  void printAST(const ConfigNode &node, int indent) const;
  void printAST(void) const;
};

#endif
