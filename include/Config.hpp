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

  // getters
  const ConfigNode &getAST()  { return ast_; };

  pair<string, int> parseListenArgument(const string &arg) const;
  pair<string, int> getListenInfo(const ConfigNode &serverNode) const;
  vector<ServerConfigue> getServerConfigues() const;

  // Debugging
  void printAST(const ConfigNode &node, int indent) const;
  void printAST(void) const;
};

#endif

// first : each socket is created and a server from the configue file is bound to it (or a map of servers)
// and to build the response we need to rout data from the server
// to rout data we need a router 
// a router need a uri
// a uri is need the string of the path , it creat class uri from it then we pass it to the router , the router return a Location instance with the default defined in router class 
// the location instance is an obj that allow us to access to a sepicifique data in a location using the [] operator , if you tried to access something that dosent exist in the map 
// an error is returend class Location will   with op [] will return a template , witch can any data type 
// For example : we need the pair of error_page that is pair(codes_list, path) , we gona do : pair<list,code> location_[error_page] or bool location_[internal] , but is this possible