#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "ServerConfig.hpp"
#include <sstream>
#include <fstream>
#include <algorithm>

class Config
{
public:
  Config();
  Config(const std::string &path);

  void load(const std::string &path);
  const ServerConfig &matchServer(int port, const std::string &host) const;
  class ConfigException : public std::runtime_error
  {
  public:
    ConfigException(const std::string &msg) : std::runtime_error(msg) {}
  };

private:
  std::vector<ServerConfig> servers;

  void parse(std::istream &in);
  void validate() const;
};

#endif