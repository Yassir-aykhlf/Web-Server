#include "Config.hpp"

Config::Config(string filename) : filename_(filename) {};

// Debugging
// ─── ANSI Colors ──────────────────────────────────
#define COLOR_RESET "\033[0m"
#define COLOR_CYAN "\033[36m"   // Simple directives (listen, root, method...)
#define COLOR_RED "\033[31m"    // Block directives (http, server, location)
#define COLOR_GREEN "\033[32m"  // Values / parameters
#define COLOR_GRAY "\033[90m"   // Braces { } and semicolons ;
#define COLOR_YELLOW "\033[33m" // Location path (first arg of location)

// ───────────────────────────────────────────────────

void Config::printAST(const ConfigNode &node, int indent) const
{
    string indentation(indent * 2, ' ');

    bool isBlock = (node.getType() == ROOT || node.getType() == BLOCK);

    // Print the directive name
    cout << indentation << (isBlock ? COLOR_RED : COLOR_CYAN)
         << node.getName() << COLOR_RESET;

    // Print arguments on the same line
    if (!node.getArguments().empty())
    {
        cout << " ";
        for (size_t i = 0; i < node.getArguments().size(); ++i)
        {
            // Location path gets yellow, everything else green
            if (node.getName() == "location" && i == 0)
                cout << COLOR_YELLOW;
            else
                cout << COLOR_GREEN;

            cout << node.getArguments()[i] << COLOR_RESET;

            if (i < node.getArguments().size() - 1)
                cout << " ";
        }
    }

    // Block directive → open brace and recurse
    if (!node.getChildren().empty())
    {
        cout << " " << COLOR_GRAY << "{" << COLOR_RESET << endl;

        vector<ConfigNode> children = node.getChildren();
        for (size_t i = 0; i < children.size(); ++i)
            printAST(children[i], indent + 1);

        cout << indentation << COLOR_GRAY << "}" << COLOR_RESET
             << endl;
    }
    else
    {

        if (isBlock)
            cout << " " << COLOR_GRAY << "{" << COLOR_RESET << " "
                 << COLOR_GRAY << "}" << COLOR_RESET << endl;
        else
            cout << COLOR_GRAY << ";" << COLOR_RESET << endl;
    }
}

// Overload for easy usage
void Config::printAST(void) const
{
    printAST(ast_, 0);
}

void Config::load()
{
    try
    {
        ConfigParser parser;
        ast_ = parser.parse(filename_);

        ConfigValidator validator;
        validator.validate(ast_);
    }
    catch (const ParseException &e)
    {
        throw ConfigException(string("Parsing error: ") + e.what());
    }
    catch (const ConfigException &e)
    {
        throw;
    }
}
