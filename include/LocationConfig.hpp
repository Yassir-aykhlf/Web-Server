#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <string>

class LocationConfig
{
public:
  std::string path;     // e.g. "/images" . . .
  std::string root;     // filesystem root
  std::string cgi_pass; // empty if not CGI
  
};

#endif // LOCATIONCONFIG_HPP