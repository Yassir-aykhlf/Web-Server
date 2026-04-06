#include "ConfigParser.hpp"
using namespace std;

ConfigParser::ConfigParser() {}

ConfigParser::~ConfigParser()
{
    if (stream_.is_open())
    {
        stream_.close();
    }
}

void ConfigParser::openFile(const string &filename)
{
    filename_ = filename;
    if (stream_.is_open())
        stream_.close();
    stream_.open(filename.c_str());

    if (!stream_.is_open())
        throw FileException("Cannot open file : " + filename);
}

char ConfigParser::peekChar() {
    return stream_.peek();
}

char ConfigParser::getChar() {
    return stream_.get();
}

void ConfigParser::skipWhitespace() {
    while (stream_ && isspace(peekChar())) {
        getChar();
    }
}

void ConfigParser::skipComments() {
    while (stream_) {
        skipWhitespace();
        if (peekChar() != '#')
            break;
        while (stream_ && getChar() != '\n')
            ;
    }
}

Token ConfigParser::readSingleCharToken()
{
    char ch = peekChar();
    switch (ch)
    {
    case ';':
        getChar();
        return Token(SEMICOLON, ";");
    case '{':
        getChar();
        return Token(LBRACE, "{");
    case '}':
        getChar();
        return Token(RBRACE, "}");
    default:
        throw ParseException(string("Unexpected character '") + ch + "'" + filename_);
    }
}

Token ConfigParser::readWordToken()
{
    string value;
    while (stream_ && !isspace(peekChar()) &&
           peekChar() != ';' && peekChar() != '{' &&
           peekChar() != '}' && peekChar() != '#')
    {
        if (stream_.eof())
            break;
        value += getChar();
    }
    return Token(WORD, value);
}

Token ConfigParser::readNextToken()
{
    skipComments();
    if (stream_.eof())
        return Token(EOS, "");
    char ch = peekChar();
    if (ch == ';' || ch == '{' || ch == '}')
        return readSingleCharToken();
    return readWordToken();
}

void ConfigParser::advance()
{
    currentToken_ = readNextToken();
}

bool ConfigParser::match(TokenType type) const
{
    return currentToken_.type == type;
}

Token ConfigParser::expect(TokenType type)
{
    if (currentToken_.type != type)
    {
        ostringstream oss;
        oss << "Expected ";
        switch (type)
        {
        case WORD:
            oss << "word";
            break;
        case SEMICOLON:
            oss << "';'";
            break;
        case LBRACE:
            oss << "'{'";
            break;
        case RBRACE:
            oss << "'}'";
            break;
        case EOS:
            oss << "end of file";
            break;
        }
        oss << " but got '" << currentToken_.value << "'";
        throw ParseException(oss.str() + filename_);
    }
    Token token = currentToken_;
    advance();
    return token;
}

ConfigNode ConfigParser::parseDirective()
{
    Token nameToken = expect(WORD);
    ConfigNode node(SIMPLE, nameToken.value);

    while (!match(SEMICOLON) && !match(LBRACE))
    {
        if (match(EOS))
            throw ParseException("Unexpected end of file in directive '" + nameToken.value + "'");
        Token arg = expect(WORD);
        node.addArgument(arg.value);
    }

    if (match(SEMICOLON))
    {
        advance();
        return node;
    }

    if (match(LBRACE))
    {
        advance();
        node.setType(BLOCK);

        parseContext(node);

        expect(RBRACE);
        return node;
    }

    throw ParseException("Expected ';' or '{'" + filename_);
}

void ConfigParser::parseContext(ConfigNode &parent)
{
    while (!match(EOS) && !match(RBRACE))
    {
        ConfigNode directive = parseDirective();
        parent.addChild(directive);
    }
}

ConfigNode ConfigParser::parse(const string filename)
{
    openFile(filename);
    advance();
    ConfigNode root = ConfigNode(ROOT, "http");
    parseContext(root);
    stream_.close();
    return root;
}
