#include "URI.hpp"
using namespace std;

URI::URI(const string& uri)
{
  parse(uri);
}

void URI::parse(const string& uri)
{
  if (uri.empty())
  {
    path_ = "/";
    parseSegments();
    return;
  }

  string temp = uri;

  // Extract fragment (after #)
  size_t fragmentPos = temp.find('#');
  if (fragmentPos != string::npos)
  {
    fragment_ = temp.substr(fragmentPos + 1);
    temp = temp.substr(0, fragmentPos);
  }

  // Extract query (after ?)
  size_t queryPos = temp.find('?');
  if (queryPos != string::npos)
  {
    query_ = temp.substr(queryPos + 1);
    temp = temp.substr(0, queryPos);
  }

  // What's left is the path
  path_ = temp.empty() ? "/" : temp;
  
  // Parse segments
  parseSegments();
}

void URI::parseSegments()
{
  segments_.clear();
  
  if (path_.empty() || path_ == "/")
    return;
  
  stringstream ss(path_);
  string segment;
  
  while (getline(ss, segment, '/'))
  {
    if (!segment.empty())
      segments_.push_back(segment);
  }
}

string URI::getNormalizedPath() const
{
  if (path_ == "/")
    return "/";

  vector<string> normalizedSegments;

  for (size_t i = 0; i < segments_.size(); i++)
  {
    const string& seg = segments_[i];
    
    if (seg == ".")
      continue;  // Skip current directory
    
    if (seg == "..")
    {
      // Go up one level if possible
      if (!normalizedSegments.empty())
        normalizedSegments.pop_back();
    }
    else
    {
      normalizedSegments.push_back(seg);
    }
  }

  // Rebuild path
  if (normalizedSegments.empty())
    return "/";

  string result;
  for (size_t i = 0; i < normalizedSegments.size(); i++)
    result += "/" + normalizedSegments[i];

  return result;
}

string URI::getExtension() const
{
  if (segments_.empty())
    return "";
  
  const string& lastSegment = segments_[segments_.size() - 1];
  size_t dotPos = lastSegment.find_last_of('.');
  
  if (dotPos != string::npos && dotPos > 0)
    return lastSegment.substr(dotPos);
  
  return "";
}

bool URI::isDirectory() const
{
  return !path_.empty() && path_[path_.length() - 1] == '/';
}
