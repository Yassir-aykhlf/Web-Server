#pragma once
#include <string>
#include <sstream>
#include <vector>

using namespace std;

class URI
{
private:
  string path_;
  string query_;
  string fragment_;
  vector<string> segments_;
  
  void parseSegments();

public:
  URI() : path_("/"), query_(""), fragment_("") {}

  // Parse from full URI string
  URI(const string& uri);

  void parse(const string& uri);
  
  const string& getPath() const { return path_; }
  const string& getQuery() const { return query_; }
  const string& getFragment() const { return fragment_; }
  
  // Get path segments (e.g. "/upload/images" → ["upload", "images"])
  const vector<string>& getSegments() const { return segments_; }
  
  // Get normalized path (removes . and .. and multiple slashes)
  string getNormalizedPath() const;
  
  // Get file extension from path
  string getExtension() const;
  
  // Check if path is root
  bool isRoot() const { return path_ == "/"; }
  
  // Check if path points to a directory (ends with /)
  bool isDirectory() const;
  
  // Get number of segments
  size_t segmentCount() const { return segments_.size(); }
};
