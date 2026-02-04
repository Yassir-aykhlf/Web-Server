#pragma once
#include <string>
#include <vector>

using namespace std;

enum NodeType
{
  ROOT,
  BLOCK,
  SIMPLE
};

class ConfigNode
{
private:
  NodeType type_;
  string name_;
  vector<string> arguments_;
  vector<ConfigNode> children_;

public:
  // Constructor
  ConfigNode(NodeType type = ROOT, const string &name = "");

  // Getters
  NodeType getType() const;
  const string &getName() const;
  const vector<string> &getArguments() const;
  const vector<ConfigNode> &getChildren() const;

  // Setters
  void setType(NodeType type);
  void setName(const string &name);
  void addArgument(const string &arg);
  void addChild(ConfigNode &child);
};
