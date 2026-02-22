#pragma once
#include <string>
#include <sstream>
#include <vector>

class URI
{
private:
  std::string path_;
  std::string query_;
  std::string fragment_;
  std::vector<std::string> segments_;
  
  void parseSegments();

public:
  URI() : path_("/"), query_(""), fragment_("") {}

  URI(const std::string& uri);

  void parse(const std::string& uri);
  
  const std::string& getPath() const { return path_; }
  const std::string& getQuery() const { return query_; }
  const std::string& getFragment() const { return fragment_; }
  
  // Get path segments (e.g. "/upload/images" → ["upload", "images"])
  const std::vector<std::string>& getSegments() const { return segments_; }
  
  // Get normalized path (removes . and .. and multiple slashes)
  std::string getNormalizedPath() const;
  
  // Get file extension from path
  std::string getExtension() const;
  
  // Check if path is root
  bool isRoot() const { return path_ == "/"; }
  
  // Check if path points to a directory (ends with /)
  bool isDirectory() const;
  
  // Get number of segments
  size_t segmentCount() const { return segments_.size(); }
};
