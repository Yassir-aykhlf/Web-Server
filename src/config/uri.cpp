#include "URI.hpp"

URI::URI(const string &path) : path_(path) { parsePath(); };

void URI::parsePath()
{
  size_t start = 0;
  size_t end = path_.find('/');
  while (end != string::npos)
  {
    if (end != start) // Avoid empty segments
      segments_.push_back(path_.substr(start, end - start));
    start = end + 1;
    end = path_.find('/', start);
  }
  if (start < path_.length()) // Add the last segment
    segments_.push_back(path_.substr(start));
}
