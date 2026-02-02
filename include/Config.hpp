#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "ConfigNode.hpp"
#include "ConfigParser.hpp"
#include "ConfigExceptions.hpp"
#include "ConfigValidator.hpp"

class Config
{
private:
  ConfigNode ast_;
  std::string filename_;

public:
  Config(const std::string filename);
  void load();

  // getters
  const ConfigNode &getAST() const { return ast_; };

  // Debugging
  void printAST(const ConfigNode &node, int indent) const;
  void printAST(void) const;
};

#endif