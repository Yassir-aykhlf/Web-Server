#pragma once
#include <string>
#include "ConfigNode.hpp"
#include <vector>

class Location;

class ConfigValue
{
private:
  std::string key_;
  const Location *location_;

public:
  ConfigValue(const std::string &key, const Location *loc);
  operator std::string() const;
  operator bool() const;
  operator std::vector<std::string>() const;
  operator std::pair<std::vector<int>, std::string>() const;
  operator std::pair<int, std::string>() const;
};

class Location
{
private:
  ConfigNode serverNode_;
  ConfigNode location_;

  const ConfigNode *findDirective(const ConfigNode &node, const std::string &key) const;
  const std::vector<std::string> *getArguments(const std::string &key) const;
  std::string getDefaultString(const std::string &key) const;
  bool getDefaultBool(const std::string &key) const;
  std::vector<std::string> getDefaultList(const std::string &key) const;

public:
  Location();
  Location &operator=(const Location &other);
  Location(const ConfigNode &loc, const ConfigNode &server);

  std::string getStringValue(const std::string &key) const;
  bool getBoolValue(const std::string &key) const;
  std::vector<std::string> getListValue(const std::string &key) const;
  std::pair<std::vector<int>, std::string> getPairValue(const std::string &key) const;
  std::pair<int, std::string> getPairVal(const std::string &key) const;
  std::string findErrorPagePath(int statusCode) const;
  ConfigValue operator[](const std::string &key) const;
};

