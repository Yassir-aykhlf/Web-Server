#pragma once

#include "ConfigExceptions.hpp"
#include "ConfigNode.hpp"
#include "ConfigParser.hpp"
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>

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
    TokenType   type;
    std::string value;

    Token() : type(EOS) {}

    Token(TokenType t, const std::string &v) : type(t), value(v) {}
};

class ConfigParser
{
  private:
    std::ifstream stream_;
    Token         currentToken_;
    std::string   filename_;

    // ========================================
    // Tokenization (private methods)
    // ========================================
    char peekChar();
    char getChar();
    void skipWhitespace();
    void skipComments();

    Token readSingleCharToken();
    Token readWordToken();
    Token readNextToken();

    // ========================================
    // Token interface (private)
    // ========================================
    void  advance();
    bool  match(TokenType type) const;
    Token expect(TokenType type);

    // ========================================
    // Parsing (private methods)
    // ========================================
    void       openFile(const std::string &filename);
    void       parseContext(ConfigNode &parent);
    ConfigNode parseDirective();

  public:
    ConfigParser();
    ~ConfigParser();

    ConfigNode parse(const std::string filename);
};