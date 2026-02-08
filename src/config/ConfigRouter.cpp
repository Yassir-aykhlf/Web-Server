#include "ConfigRouter.hpp"

ConfigRouter::ConfigRouter(const ServerConfigue &serverConf)
    : serverConf_(serverConf), trieRoot_(NULL)
{
  buildLocationTrie();
}

ConfigRouter::~ConfigRouter()
{
  delete trieRoot_;
}

const ServerConfigue& ConfigRouter::getServerConfig() const
{
  return serverConf_;
}

void ConfigRouter::buildLocationTrie()
{
  // Create root node
  trieRoot_ = new LocationTrieNode();
  trieRoot_->serverNode = serverConf_.getNode();
  
  // Get all children of server node
  const ConfigNode& serverNode = serverConf_.getNode();
  const vector<ConfigNode>& children = serverNode.getChildren();
  
  // Find all location blocks and insert them
  for (size_t i = 0; i < children.size(); i++)
  {
    if (children[i].getName() == "location")
    {
      const vector<string>& args = children[i].getArguments();
      if (!args.empty())
      {
        // Use URI to parse the location path
        URI locationURI(args[0]);
        insertLocationByURI(locationURI, children[i]);
      }
    }
  }
}

void ConfigRouter::insertLocationByURI(const URI& uri, const ConfigNode& locationNode)
{
  // Special case: location /
  if (uri.isRoot())
  {
    trieRoot_->isEndOfPath = true;
    trieRoot_->locationNode = locationNode;
    trieRoot_->serverNode = serverConf_.getNode();
    return;
  }
  
  // Get segments from URI (already parsed and split)
  const vector<string>& segments = uri.getSegments();
  
  // Start from root and traverse/create nodes
  LocationTrieNode* current = trieRoot_;
  
  for (size_t i = 0; i < segments.size(); i++)
  {
    const string& seg = segments[i];

    cout << "Inserting segment: " << seg << endl;
    
    // Look for existing child with this segment
    LocationTrieNode* child = NULL;
    for (size_t j = 0; j < current->children.size(); j++)
    {
      if (current->children[j]->pathSegment == seg)
      {
        child = current->children[j];
        break;
      }
    }
    
    // If not found, create new child
    if (!child)
    {
      child = new LocationTrieNode();
      child->pathSegment = seg;
      child->serverNode = serverConf_.getNode();
      child->isEndOfPath = false;
      current->children.push_back(child);
    }
    
    current = child;
  }
  
  // Mark the final node as end of location path
  current->isEndOfPath = true;
  current->locationNode = locationNode;
  current->serverNode = serverConf_.getNode();
}

LocationTrieNode* ConfigRouter::findBestMatchByURI(const URI& uri) const
{
  if (!trieRoot_)
    return NULL;
  
  // Handle root path
  if (uri.isRoot())
  {
    if (trieRoot_->isEndOfPath)
      return trieRoot_;
    return NULL;
  }
  
  // Get segments from URI
  const vector<string>& segments = uri.getSegments();
  
  // Traverse trie and track last match (longest prefix)
  LocationTrieNode* current = trieRoot_;
  LocationTrieNode* lastMatch = NULL;
  
  // Check if root is a match
  if (current->isEndOfPath)
    lastMatch = current;
  
  // Walk through segments
  for (size_t i = 0; i < segments.size(); i++)
  {
    const string& seg = segments[i];
    
    // Find child with this segment
    LocationTrieNode* child = NULL;
    for (size_t j = 0; j < current->children.size(); j++)
    {
      if (current->children[j]->pathSegment == seg)
      {
        child = current->children[j];
        break;
      }
    }
    
    // No child found, stop here
    if (!child)
      break;
    
    current = child;
    
    // If this node is end of a location, remember it (longest prefix match)
    if (current->isEndOfPath)
      lastMatch = current;
  }
  
  return lastMatch;
}

Location ConfigRouter::route(const string& path)
{
  // Parse URI from path string
  URI uri(path);
  
  // Normalize the path (removes .., ., and more :) )
  string normalizedPath = uri.getNormalizedPath();
  
  // Create URI from normalized path for matching
  URI normalizedURI(normalizedPath);
  
  // Find best matching location using normalized URI
  LocationTrieNode* match = findBestMatchByURI(normalizedURI);
  
  // If match found, create Location from it
  if (match && match->isEndOfPath)
    return Location(match->locationNode, serverConf_.getNode());
  
  // No match found - return empty Location
  return Location(ConfigNode(), serverConf_.getNode());
}


//TODO :
// 0 - understand how routing works 
// 1 - Fix the location class check the defaults and how it's go back to the server
// 2 - implement the virtuale servers
// 3 - merge with main and test