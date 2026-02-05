#pragma once

#include <string>
#include "ConfigNode.hpp"
#include "ServerConfigue.hpp"
#include <iostream>
#include <vector>

using namespace std;

class ConfigValue;
class Location
{
private:
  ConfigNode serverNode_;
  ConfigNode location_;

  // Helper to search for a directive in a given node
  const ConfigNode *findDirective(const ConfigNode &node, const string &key) const
  {
    const vector<ConfigNode> &children = node.getChildren();
    for (size_t i = 0; i < children.size(); i++)
    {
      if (children[i].getName() == key)
        return &children[i];
    }
    return NULL;
  }

  // Helper to get arguments with fallback: location → server → default
  const vector<string> *getArguments(const string &key) const
  {
    // 1. Look in location
    const ConfigNode *directive = findDirective(location_, key);
    if (directive)
      return &directive->getArguments();

    // 2. Fall back to server
    directive = findDirective(serverNode_, key);
    if (directive)
      return &directive->getArguments();

    // 3. Not found
    return NULL;
  }

public:
  Location() {};
  Location &operator=(const Location &other)
  {
    if (this != &other)
    {
      serverNode_ = other.serverNode_;
      location_ = other.location_;
    }
    return *this;
  }
  Location(const ConfigNode &loc, const ConfigNode &server)
      : serverNode_(server), location_(loc) {}

  string getStringValue(const string &key) const
  {
    const vector<string> *args = getArguments(key);
    if (args && !args->empty())
      return (*args)[0];

    return getDefaultString(key);
  }

  bool getBoolValue(const string &key) const
  {
    const vector<string> *args = getArguments(key);
    if (args && !args->empty())
    {
      string value = (*args)[0];
      return (value == "on" || value == "true" || value == "1");
    }

    return getDefaultBool(key);
  }

  vector<string> getListValue(const string &key) const
  {
    const vector<string> *args = getArguments(key);
    if (args)
      return *args;

    return getDefaultList(key);
  }

  pair<vector<int>, string> getPairValue(const string &key) const
  {
    const vector<string> *args = getArguments(key);
    if (args && args->size() >= 2)
    {
      string path = (*args)[args->size() - 1];
      vector<int> codes;
      for (size_t i = 0; i < args->size() - 1; i++)
        codes.push_back(std::atoi((*args)[i].c_str()));

      return make_pair(codes, path);
    }

    return make_pair(vector<int>(), "");
  }

  pair<int, string> getPairVal(const string &key) const
  {
    const vector<string> *args = getArguments(key);
    if (args && !args->empty())
    {
      int status = std::atoi((*args)[0].c_str());
      string url = (args->size() > 1) ? (*args)[1] : "";
      return make_pair(status, url);
    }

    return make_pair(0, "");
  }

  ConfigValue operator[](const string &key) const
  {
    return ConfigValue(key, this);
  }

private:
  string getDefaultString(const string &key) const
  {
    if (key == "root")
      return "/var/www/html";
    if (key == "upload_store")
      return "";
    return "";
  }

  bool getDefaultBool(const string &key) const
  {
    if (key == "autoindex")
      return false;
    if (key == "internal")
      return false;
    return false;
  }

  vector<string> getDefaultList(const string &key) const
  {
    if (key == "index")
    {
      vector<string> defaults;
      defaults.push_back("index.html");
      return defaults;
    }
    if (key == "method")
    {
      vector<string> defaults;
      defaults.push_back("GET");
      return defaults;
    }
    return vector<string>();
  }
};

class ConfigValue
{
private:
  string key_;
  const Location *location_;

public:
  ConfigValue(const string &key, const Location *loc)
      : key_(key), location_(loc) {}

  operator string() const
  {
    return location_->getStringValue(key_);
  }

  operator bool() const
  {
    return location_->getBoolValue(key_);
  }

  operator vector<string>() const
  {
    return location_->getListValue(key_);
  }

  operator pair<vector<int>, string>() const
  {
    return location_->getPairValue(key_);
  }

  operator pair<int, string>() const
  {
    return location_->getPairVal(key_);
  }
};

// so configuerouter will have serverConfigue instance and route method takes URI class instance , that return a Location that can accessed using op[] ,
//!  So each instance of confrouter is a router to a sepicifique location

//? what about storing fd socket at the server configue ?
// TODO : understand why He store the fd of the socket in the server configue , and if it wrong fix it