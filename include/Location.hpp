#pragma once
#include <string>
#include "ConfigNode.hpp"
#include <iostream>
#include <vector>
using namespace std;

// Forward declare Location
class Location;

class ConfigValue
{
private:
  string key_;
  const Location *location_;

public:
  ConfigValue(const string &key, const Location *loc);
  operator string() const;
  operator bool() const;
  operator vector<string>() const;
  operator pair<vector<int>, string>() const;
  operator pair<int, string>() const;
};

class Location
{
private:
  ConfigNode serverNode_;
  ConfigNode location_;

  const ConfigNode *findDirective(const ConfigNode &node, const string &key) const;
  const vector<string> *getArguments(const string &key) const;
  string getDefaultString(const string &key) const;
  bool getDefaultBool(const string &key) const;
  vector<string> getDefaultList(const string &key) const;

public:
  Location();
  Location &operator=(const Location &other);
  Location(const ConfigNode &loc, const ConfigNode &server);

  string getStringValue(const string &key) const;
  bool getBoolValue(const string &key) const;
  vector<string> getListValue(const string &key) const;
  pair<vector<int>, string> getPairValue(const string &key) const;
  pair<int, string> getPairVal(const string &key) const;
  ConfigValue operator[](const string &key) const;
};

