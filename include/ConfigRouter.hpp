#pragma once
#include "ConfigNode.hpp"
#include "ServerConfig.hpp"
#include "Location.hpp"
#include "URI.hpp"
#include <string>
#include <vector>
#include <map>

struct LocationTrieNode
{
  std::string pathSegment;
  ConfigNode locationNode;
  ConfigNode serverNode;
  std::vector<LocationTrieNode *> children;
  bool isEndOfPath;

  LocationTrieNode()
      : pathSegment(""), isEndOfPath(false) {}

  ~LocationTrieNode()
  {
    for (size_t i = 0; i < children.size(); i++)
      delete children[i];
  }
};

class ConfigRouter
{
private:
  const ServerConfig &serverConf_;
  std::map<std::string, ConfigNode> serverNodeMap_;
  LocationTrieNode *trieRoot_;
  std::string serverName_;
  
  void buildServerNodeMap();
  void buildLocationTrie();
  void insertLocationByURI(const URI &uri, const ConfigNode &locationNode);
  LocationTrieNode *findBestMatchByURI(const URI &uri) const;

public:
  ConfigRouter(const ServerConfig &serverConf, const std::string &serverName);
  ConfigRouter(const ServerConfig &serverConf);
  ~ConfigRouter();

  Location route(const std::string &path);
  const ServerConfig &getServerConfig() const;
};
