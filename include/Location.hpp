#pragma once
#include <string>
#include "ConfigNode.hpp"
#include <vector>

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

  std::string getPath() const;
  bool hasLocalDirective(const std::string &key) const;
  std::string getStringValue(const std::string &key) const;
  bool getBoolValue(const std::string &key) const;
  std::vector<std::string> getListValue(const std::string &key) const;
  std::pair<int, std::string> getPairVal(const std::string &key) const;
  std::string findErrorPagePath(int statusCode) const;
};
