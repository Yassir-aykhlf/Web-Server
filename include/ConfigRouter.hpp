#pragma once
#include "ConfigNode.hpp"
#include "ServerConfigue.hpp"
#include "Location.hpp"
#include "URI.hpp"
#include <string>
#include <vector>

using namespace std;

struct LocationTrieNode
{
  string pathSegment;
  ConfigNode locationNode;
  ConfigNode serverNode;
  vector<LocationTrieNode*> children;
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
  const ServerConfigue& serverConf_;
  LocationTrieNode* trieRoot_;
  
  void buildLocationTrie();
  void insertLocationByURI(const URI& uri, const ConfigNode& locationNode);
  LocationTrieNode* findBestMatchByURI(const URI& uri) const;

public:
  ConfigRouter(const ServerConfigue &serverConf);
  ~ConfigRouter();
  
  Location route(const string& path);
  const ServerConfigue& getServerConfig() const;
};
