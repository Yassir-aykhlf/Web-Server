#pragma once
#include <string>
#include <vector>

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
  std::string name_;
  std::vector<std::string> arguments_;
  std::vector<ConfigNode> children_;

public:
  ConfigNode(NodeType type = ROOT, const std::string &name = "");

  // Getters
  NodeType getType() const;
  const std::string &getName() const;
  const std::vector<std::string> &getArguments() const;
  const std::vector<ConfigNode> &getChildren() const;

  // Setters
  void setType(NodeType type);
  void setName(const std::string &name);
  void addArgument(const std::string &arg);
  void addChild(ConfigNode &child);
};
