#pragma once

#include <string>
#include <vector>

// Let i first make th uri class that will help me to manage the path and get the segments of it , then i will make the trie structuer and fill it with all locations of this server , then i will make the route method that will traverse the tree and return the match location or unavailable
using namespace std;

class URI
{
private:
  string path_;
  vector<string> segments_;

  void parsePath();

public:
  URI(const string &path) : path_(path) { parsePath(); };
};

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

