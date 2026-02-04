#pragma once

#include <string>
#include "ConfigNode.hpp"
#include <iostream>
#include <vector>

using namespace std;

class Location
{
public:
  string getStringValue(const string &key) const;
  bool getBoolValue(const string &key) const;
  vector<string> getListValue(const string &key) const;
  pair<vector<int>, string> getPairValue(const string &key) const;
  // If you need other pair types later, overload it or make it a template

  ConfigValue operator[](const string &key) const
  {
    return ConfigValue(key, this);
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
};