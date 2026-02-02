#include "Config.hpp"

Config::Config(std::string filename) : filename_(filename) {};

// Debugging
void Config::printAST(const ConfigNode &node, int indent) const
{
        std::string indentation(indent * 2, ' ');

        std::cout << indentation << "Directive: " << node.getName() << std::endl;

        // Print parameters
        if (!node.getArguments().empty())
        {
                std::cout << indentation << "Parameters: ";
                for (size_t i = 0; i < node.getArguments().size(); ++i)
                {
                        std::cout << node.getArguments()[i];
                        if (i < node.getArguments().size() - 1)
                        {
                                std::cout << ", ";
                        }
                }
                std::cout << std::endl;
        }

        // Print children recursively

        if (!node.getChildren().empty())
        {
                std::cout << indentation << "Children {" << std::endl;
                for (size_t i = 0; i < node.getChildren().size(); ++i)
                {
                        std::vector<ConfigNode> children = node.getChildren();
                        printAST(children[i], indent + 1);
                }
                std::cout << indentation << "}" << std::endl;
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
                throw ConfigException(std::string("Parsing error: ") + e.what());
        }
        catch (const ConfigException &e)
        {
                throw;
        }
}
