#include "Location.hpp"
#include <cstdlib>
using namespace std;

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

string Location::getPath() const
{
  const vector<string> &args = location_.getArguments();
  if (!args.empty())
    return args[0];
  return "/";
}

bool Location::hasLocalDirective(const std::string &key) const
{
  return findDirective(location_, key) != NULL;
}

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

string Location::findErrorPagePath(int statusCode) const
{
  // Search in location node first, then server node
  const ConfigNode *nodes[] = {&location_, &serverNode_};
  for (int n = 0; n < 2; n++)
  {
    const vector<ConfigNode> &children = nodes[n]->getChildren();
    for (size_t i = 0; i < children.size(); i++)
    {
      if (children[i].getName() == "error_page")
      {
        const vector<string> &args = children[i].getArguments();
        if (args.size() >= 2)
        {
          string path = args[args.size() - 1];
          for (size_t j = 0; j < args.size() - 1; j++)
          {
            if (std::atoi(args[j].c_str()) == statusCode)
              return path;
          }
        }
      }
    }
  }
  return "";
}

string Location::getDefaultString(const string &key) const
{
  if (key == "root")
    return "/var/www/html";
  if (key == "client_max_body_size")
    return "100m";
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
