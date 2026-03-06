#pragma once
#include <string>
#include <sstream>
#include <vector>

class URI
{
private:
  std::string path_;
  std::vector<std::string> segments_;
  
  void parseSegments();

public:
  URI() : path_("/") {}
  URI(const std::string& uri);

  void parse(const std::string& uri);
  
  const std::string& getPath() const { return path_; }
  const std::vector<std::string>& getSegments() const { return segments_; }
  std::string getNormalizedPath() const;
  bool isRoot() const { return path_ == "/"; }
};
