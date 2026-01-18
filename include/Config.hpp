#ifndef CONFIG_HPP
#define CONFIG_HPP


#include "ServerConfig.hpp"
#include <stdexcept>


class Config
{
public:
  Config();
  Config(const std::string& path);
  
  void load(const std::string &path);
  const ServerConfig &matchServer(int port, const std::string &host) const;
  class ConfigException : public std::runtime_error
  {
  public:
    ConfigException(const std::string &msg): std::runtime_error(msg) {}
  };

private:
  std::vector<ServerConfig> servers;

  void parse(std::istream &in);
  void validate() const;
};

//! i think there is no need for a configparser.hpp/cpp file

#endif // CONFIG_HPP