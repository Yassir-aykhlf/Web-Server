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

public:
  Config(const string filename);
  void load();

  const ConfigNode &getAST()  { return ast_; };

  pair<string, int> parseListenArgument(const string &arg) const;
  vector<pair<string, int> > getAllListenInfo(const ConfigNode &serverNode) const;
  vector<string> getServerNames(const ConfigNode &serverNode) const;
  vector<ServerConfigue> getServerConfigues() const;

  // Debugging
  void printAST(const ConfigNode &node, int indent) const;
  void printAST(void) const;
};

#endif
