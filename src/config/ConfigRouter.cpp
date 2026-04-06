#include "ConfigRouter.hpp"
using namespace std;

void ConfigRouter::buildServerNodeMap()
{
  const vector<ConfigNode> &nodes = serverConf_.getNodes();
  bool foundServerName = false;

  for (size_t i = 0; i < nodes.size(); i++)
  {
    const ConfigNode &node = nodes[i];
    vector<string> serverNames = serverConf_.getServerNames(node);
    for (size_t j = 0; j < serverNames.size(); j++)
    {
      const string &name = serverNames[j];
      if (name == serverName_)
        foundServerName = true;
      if (serverNodeMap_.find(name) == serverNodeMap_.end())
      {
        serverNodeMap_[name] = node;
      }
    }
  }
  if (!foundServerName && !serverNodeMap_.empty())
    serverName_ = serverNodeMap_.begin()->first;
}

ConfigRouter::ConfigRouter(const ServerConfig &serverConf, const string &serverName)
    : serverConf_(serverConf), trieRoot_(NULL), serverName_(serverName)
{
  buildServerNodeMap();
  buildLocationTrie();
}

ConfigRouter::ConfigRouter(const ServerConfig &serverConf)
    : serverConf_(serverConf), trieRoot_(NULL), serverName_("")
{
  buildServerNodeMap();
  buildLocationTrie();
}

ConfigRouter::~ConfigRouter() {
  delete trieRoot_;
}

const ServerConfig &ConfigRouter::getServerConfig() const {
  return serverConf_;
}

void ConfigRouter::buildLocationTrie()
{
  trieRoot_ = new LocationTrieNode();
  trieRoot_->serverNode = serverNodeMap_[serverName_];
  const ConfigNode &serverNode = serverNodeMap_[serverName_];
  const vector<ConfigNode> &children = serverNode.getChildren();
  for (size_t i = 0; i < children.size(); i++)
  {
    if (children[i].getName() == "location")
    {
      const vector<string> &args = children[i].getArguments();
      if (!args.empty())
      {
        URI locationURI(args[0]);
        insertLocationByURI(locationURI, children[i]);
      }
    }
  }
}

void ConfigRouter::insertLocationByURI(const URI &uri, const ConfigNode &locationNode)
{
  if (uri.isRoot())
  {
    trieRoot_->isEndOfPath = true;
    trieRoot_->locationNode = locationNode;
    trieRoot_->serverNode = serverNodeMap_[serverName_];
    return;
  }
  const vector<string> &segments = uri.getSegments();
  LocationTrieNode *current = trieRoot_;
  for (size_t i = 0; i < segments.size(); i++)
  {
    const string &seg = segments[i];
    LocationTrieNode *child = NULL;
    for (size_t j = 0; j < current->children.size(); j++)
    {
      if (current->children[j]->pathSegment == seg)
      {
        child = current->children[j];
        break;
      }
    }
    if (!child)
    {
      child = new LocationTrieNode();
      child->pathSegment = seg;
      child->serverNode = serverNodeMap_[serverName_];
      child->isEndOfPath = false;
      current->children.push_back(child);
    }
    current = child;
  }
  current->isEndOfPath = true;
  current->locationNode = locationNode;
  current->serverNode = serverNodeMap_[serverName_];
}

LocationTrieNode *ConfigRouter::findBestMatchByURI(const URI &uri) const
{
  if (!trieRoot_)
    return NULL;

  if (uri.isRoot())
  {
    if (trieRoot_->isEndOfPath)
      return trieRoot_;
    return NULL;
  }
  const vector<string> &segments = uri.getSegments();
  LocationTrieNode *current = trieRoot_;
  LocationTrieNode *lastMatch = NULL;
  if (current->isEndOfPath)
    lastMatch = current;
  for (size_t i = 0; i < segments.size(); i++)
  {
    const string &seg = segments[i];
    LocationTrieNode *child = NULL;
    for (size_t j = 0; j < current->children.size(); j++)
    {
      if (current->children[j]->pathSegment == seg)
      {
        child = current->children[j];
        break;
      }
    }
    if (!child)
      break;
    current = child;
    if (current->isEndOfPath)
      lastMatch = current;
  }
  return lastMatch;
}

Location ConfigRouter::route(const string &path)
{
  URI uri(path);
  string normalizedPath = uri.getNormalizedPath();
  URI normalizedURI(normalizedPath);
  LocationTrieNode *match = findBestMatchByURI(normalizedURI);
  if (match && match->isEndOfPath)
    return Location(match->locationNode, serverNodeMap_[serverName_]);
  return Location(ConfigNode(), serverNodeMap_[serverName_]);
}