#include "Location.hpp"
#include <cstdlib>

// ============================================================
// Location implementations
// ============================================================
Location::Location() {}

Location &Location::operator=(const Location &other)
{
  if (this != &other)
  {
    serverNode_ = other.serverNode_;
    location_ = other.location_;
  }
  return *this;
}

Location::Location(const ConfigNode &loc, const ConfigNode &server)
    : serverNode_(server), location_(loc) {}

const ConfigNode *Location::findDirective(const ConfigNode &node, const string &key) const
{
  const vector<ConfigNode> &children = node.getChildren();
  for (size_t i = 0; i < children.size(); i++)
  {
    if (children[i].getName() == key)
      return &children[i];
  }
  return NULL;
}

const vector<string> *Location::getArguments(const string &key) const
{
  const ConfigNode *directive = findDirective(location_, key);
  if (directive)
    return &directive->getArguments();

  directive = findDirective(serverNode_, key);
  if (directive)
    return &directive->getArguments();

  return NULL;
}

string Location::getStringValue(const string &key) const
{
  const vector<string> *args = getArguments(key);
  if (args && !args->empty())
    return (*args)[0];
  return getDefaultString(key);
}

bool Location::getBoolValue(const string &key) const
{
  const vector<string> *args = getArguments(key);
  if (args && !args->empty())
  {
    string value = (*args)[0];
    return (value == "on" || value == "true" || value == "1");
  }
  return getDefaultBool(key);
}

vector<string> Location::getListValue(const string &key) const
{
  const vector<string> *args = getArguments(key);
  if (args)
    return *args;
  return getDefaultList(key);
}

pair<vector<int>, string> Location::getPairValue(const string &key) const
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

pair<int, string> Location::getPairVal(const string &key) const
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

ConfigValue Location::operator[](const string &key) const
{
  return ConfigValue(key, this);
}

string Location::getDefaultString(const string &key) const
{
  if (key == "root")
    return "/var/www/html";
  if (key == "client_max_body_size")
    return "1m";
  if (key == "client_body_timeout")
    return "60s";
  if (key == "cgi_timeout")
    return "30s"; 

  return "";
}

bool Location::getDefaultBool(const string &key) const
{
  if (key == "autoindex")
    return false;
  if (key == "internal")
    return false;
  return false;
}

vector<string> Location::getDefaultList(const string &key) const
{
  if (key == "index")
  {
    vector<string> defaults;
    defaults.push_back("index.html");
    return defaults;
  }
  if (key == "method")
  {
    return vector<string>();
  }
  if (key == "server_name")
  {
    return vector<string>();
  }
  if (key == "cgi_ext")
  {
    return vector<string>();
  }
  return vector<string>();
}
// ============================================================
// ConfigValue implementations
// ============================================================
ConfigValue::ConfigValue(const string &key, const Location *loc)
    : key_(key), location_(loc) {}

ConfigValue::operator string() const
{
  return location_->getStringValue(key_);
}

ConfigValue::operator bool() const
{
  return location_->getBoolValue(key_);
}

ConfigValue::operator vector<string>() const
{
  return location_->getListValue(key_);
}

ConfigValue::operator pair<vector<int>, string>() const
{
  return location_->getPairValue(key_);
}

ConfigValue::operator pair<int, string>() const
{
  return location_->getPairVal(key_);
}


// "server_name" returns str list
//   Default: "" (empty string)

// "error_page" returns pair(int list, string)
//   Default: none (no default error pages configured)

// "client_max_body_size" returns string
//   Default: "1m" (1 megabyte)

// "client_body_timeout" returns string
//   Default: "60s" (60 seconds)

// "root" returns string
//   Default: "html" or "/usr/share/nginx/html" (depends on installation)

// "index" returns str list
//   Default: ["index.html"]

// "autoindex" returns bool
//   Default: off (false)

// "method" returns str list
//   Default: none (all methods allowed if not specified)
//   Note: This isn't a standard Nginx directive - you might mean limit_except

// "return" returns pair(int, string)
//   Default: none (no default return)

// "internal" returns bool
//   Default: off (false)

// "cgi_ext" returns str list
//   Default: none (not a standard Nginx directive - custom for CGI)

// "cgi_path" returns string
//   Default: none (not a standard Nginx directive - custom for CGI)

// "cgi_timeout" returns string
//   Default: none (not a standard Nginx directive - custom for CGI)

// "upload_store" returns string
//   Default: none (not a standard Nginx directive by default)
//   Note: This is from nginx-upload-module or similar