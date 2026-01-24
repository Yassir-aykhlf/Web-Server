#include "Config.hpp"

Config::Config() : servers()
{
}

Config::Config(const std::string &path)
{
    load(path);
}

void Config::load(const std::string &path)
{
    std::ifstream file(path.c_str());
    if (!file.is_open())
        throw ConfigException("Cannot open config file: " + path);
    parse(file);
    validate();
    file.close();
}

void Config::parse(std::istream &in)
{
    (void)in;
    // Parsing logic to populate 'servers' vector
    // This is a placeholder; actual parsing code would go here
    return;
}

void Config::validate() const
{
    // Validation logic for the loaded configuration
    // This is a placeholder; actual validation code would go here
}

const ServerConfig &Config::matchServer(int port, const std::string &host) const
{
    for (std::vector<ServerConfig>::const_iterator it = servers.begin(); it != servers.end(); ++it)
    {
        if (it->port == port)
        {
            if (std::find(it->server_names.begin(), it->server_names.end(), host) != it->server_names.end())
            {
                return *it;
            }
        }
    }
    std::ostringstream oss;
    oss << "No matching server for port " << port << " and host " << host;
    throw ConfigException(oss.str());
}