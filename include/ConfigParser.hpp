#pragma once

#include "ConfigExceptions.hpp"
#include "ConfigNode.hpp"
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <iostream>

enum TokenType
{
  WORD,
  SEMICOLON,
  LBRACE,
  RBRACE,
  EOS
};

struct Token
{
  TokenType type;
  std::string value;
  Token() : type(EOS) {}
  Token(TokenType t, const std::string &v) : type(t), value(v) {}
};

class ConfigParser
{
private:
  std::ifstream stream_;
  Token currentToken_;
  std::string filename_;

  char peekChar();
  char getChar();
  void skipWhitespace();
  void skipComments();

  Token readSingleCharToken();
  Token readWordToken();
  Token readNextToken();

  void advance();
  bool match(TokenType type) const;
  Token expect(TokenType type);

  void openFile(const std::string &filename);
  void parseContext(ConfigNode &parent);
  ConfigNode parseDirective();

public:
  ConfigParser();
  ~ConfigParser();

  ConfigNode parse(const std::string filename);
};