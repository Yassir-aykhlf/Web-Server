#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include "LocationConfig.hpp"
#include <vector>
#include <map>

class ServerConfig
{
public:
  int port;
  std::vector<std::string> server_names;
  std::string root;
  std::vector<std::string> index_files;
  std::map<int, std::string> error_pages;
  size_t client_max_body_size;
  std::vector<LocationConfig> locations;
};

#endif // SERVERCONFIG_HPP