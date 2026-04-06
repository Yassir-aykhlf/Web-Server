#include "ConfigNode.hpp"
using namespace std;

ConfigNode::ConfigNode(NodeType type, const string &name)
    : type_(type), name_(name) {}

NodeType ConfigNode::getType() const { return type_; }
const string &ConfigNode::getName() const { return name_; }
const vector<string> &ConfigNode::getArguments() const { return arguments_; }
const vector<ConfigNode> &ConfigNode::getChildren() const { return children_; }

void ConfigNode::setType(NodeType type) { type_ = type; }
void ConfigNode::setName(const string &name) { name_ = name; }
void ConfigNode::addArgument(const string &arg) { arguments_.push_back(arg); }
void ConfigNode::addChild(ConfigNode &child)
{
  children_.push_back(child);
}