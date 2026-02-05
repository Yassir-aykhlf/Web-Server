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
  URI(const string &path);
};
