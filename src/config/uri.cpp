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

  size_t fragmentPos = temp.find('#');
  if (fragmentPos != string::npos)
    temp = temp.substr(0, fragmentPos);

  size_t queryPos = temp.find('?');
  if (queryPos != string::npos)
    temp = temp.substr(0, queryPos);

  path_ = temp.empty() ? "/" : temp;
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
      continue;
    
    if (seg == "..")
    {
      if (!normalizedSegments.empty())
        normalizedSegments.pop_back();
    }
    else
    {
      normalizedSegments.push_back(seg);
    }
  }

  if (normalizedSegments.empty())
    return "/";

  string result;
  for (size_t i = 0; i < normalizedSegments.size(); i++)
    result += "/" + normalizedSegments[i];

  return result;
}
