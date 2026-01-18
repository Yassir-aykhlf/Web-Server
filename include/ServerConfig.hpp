#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <vector>
#include <map>
#include <string>

struct LocationConfig
{
  std::string path;     // e.g. "/images"
  std::string root;     // filesystem root
  std::string cgi_pass; // empty if not CGI
};

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