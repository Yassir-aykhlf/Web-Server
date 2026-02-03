#include "ConfigNode.hpp"

// Constructor
ConfigNode::ConfigNode(NodeType type, const std::string &name)
    : type_(type), name_(name) {}

// Getters
NodeType ConfigNode::getType() const { return type_; }
const std::string &ConfigNode::getName() const { return name_; }
const std::vector<std::string> &ConfigNode::getArguments() const { return arguments_; }
const std::vector<ConfigNode> &ConfigNode::getChildren() const { return children_; }

//Setters
void ConfigNode::setType(NodeType type) { type_ = type; }
void ConfigNode::setName(const std::string &name) { name_ = name; }
void ConfigNode::addArgument(const std::string &arg) { arguments_.push_back(arg); }
void ConfigNode::addChild(ConfigNode &child)
{
  children_.push_back(child);
}