#include "ConfigValidator.hpp"

ConfigValidator::ConfigValidator()
{
    initializeRules();
}

void ConfigValidator::initializeRules()
{
    // Method 1: Using temporary vectors
    vector<string> empty;
    empty.push_back("");
    vector<string> httpParents;
    httpParents.push_back("http");
    vector<string> serverParents;
    serverParents.push_back("server");
    vector<string> serverLocationParents;
    serverLocationParents.push_back("server");
    serverLocationParents.push_back("location");
    vector<string> locationParents;
    locationParents.push_back("location");

    // ==========================================
    // BLOCK DIRECTIVES
    // ==========================================
    rules_["http"] = DirectiveRule(0, 0, true, empty, NULL);
    rules_["server"] = DirectiveRule(0, 0, true, httpParents, NULL);
    rules_["location"] = DirectiveRule(1, 1, true, serverParents, &ConfigValidator::validateLocation);

    // ==========================================
    // SERVER CONTEXT DIRECTIVES
    // ==========================================
    rules_["listen"] = DirectiveRule(1, 1, false, serverParents, &ConfigValidator::validateListen);
    rules_["server_name"] = DirectiveRule(1, MAX_PARAMS_SIZE, false, serverParents, &ConfigValidator::validateServerName);
    rules_["error_page"] = DirectiveRule(2, MAX_PARAMS_SIZE, false, serverLocationParents, &ConfigValidator::validateErrorPage);
    rules_["client_max_body_size"] = DirectiveRule(1, 1, false, serverLocationParents, &ConfigValidator::validateSize);
    rules_["client_body_timeout"] = DirectiveRule(1, 1, false, serverLocationParents, &ConfigValidator::validateTimeout);

    // ==========================================
    // LOCATION CONTEXT DIRECTIVES
    // ==========================================
    rules_["root"] = DirectiveRule(1, 1, false, serverLocationParents, &ConfigValidator::validatePath);
    rules_["index"] = DirectiveRule(1, MAX_PARAMS_SIZE, false, serverLocationParents, &ConfigValidator::validateIndex);
    rules_["autoindex"] = DirectiveRule(1, 1, false, serverLocationParents, &ConfigValidator::validateBoolean);
    rules_["method"] = DirectiveRule(1, MAX_PARAMS_SIZE, false, locationParents, &ConfigValidator::validateMethod);
    rules_["return"] = DirectiveRule(1, 2, false, serverLocationParents, &ConfigValidator::validateReturn);
    rules_["internal"] = DirectiveRule(0, 0, false, locationParents, &ConfigValidator::validateInternal);

    // ==========================================
    // CGI DIRECTIVES
    // ==========================================
    rules_["cgi_ext"] = DirectiveRule(1, MAX_PARAMS_SIZE, false, locationParents, &ConfigValidator::validateCgiExt);
    rules_["cgi_path"] = DirectiveRule(1, MAX_PARAMS_SIZE, false, locationParents, &ConfigValidator::validateCgiPath);
    rules_["cgi_timeout"] = DirectiveRule(1, 1, false, locationParents, &ConfigValidator::validateTimeout);

    // ==========================================
    // UPLOAD DIRECTIVES
    // ==========================================
    rules_["upload_store"] = DirectiveRule(1, 1, false, locationParents, &ConfigValidator::validateUploadStore);
}

bool ConfigValidator::isAllowedInContext(
    const string &directive, const string &context)
{
    map<string, DirectiveRule>::const_iterator ruleIt =
        rules_.find(directive);
    if (ruleIt == rules_.end())
        return false;

    const DirectiveRule &rule = ruleIt->second;
    for (size_t i = 0; i < rule.allowedContexts.size(); ++i)
    {
        if (rule.allowedContexts[i] == context)
            return true;
    }
    return false;
}

void ConfigValidator::validateNode(
    const ConfigNode &node, const string &context)
{
    map<string, DirectiveRule>::const_iterator ruleIt =
        rules_.find(node.getName());
    if (ruleIt == rules_.end())
        throw ConfigException("Unknown directive: " + node.getName());

    const DirectiveRule &rule = ruleIt->second;

    if (node.getType() == BLOCK && !rule.allowsChildren)
    {
        throw ConfigException(
            "Directive '" + node.getName() + "' does not allow child directives");
    }
    else if (node.getType() == SIMPLE && rule.allowsChildren)
    {
        throw ConfigException(
            "Directive '" + node.getName() + "' requires child directives");
    }

    if (!isAllowedInContext(node.getName(), context))
    {
        throw ConfigException(
            "Directive '" + node.getName() + "' not allowed in context '" +
            context + "'");
    }

    size_t paramCount = node.getArguments().size();
    if (paramCount < rule.minParams || paramCount > rule.maxParams)
        throw ConfigException(
            "Directive '" + node.getName() + "' unexpected number of parameters");

    if (rule.paramValidator != NULL)
    {
        if (!rule.paramValidator(node.getArguments()))
            throw ConfigException(
                "Invalid parameters for directive '" + node.getName() + "'");
    }

    if (!rule.allowsChildren && !node.getChildren().empty())
        throw ConfigException(
            "Directive '" + node.getName() + "' does not allow child directives");

    for (size_t i = 0; i < node.getChildren().size(); ++i)
        validateNode(node.getChildren()[i], node.getName());
}

void ConfigValidator::validate(const ConfigNode &root)
{
    validateNode(root, "");
}

// ==========================================
// VALIDATOR IMPLEMENTATIONS
// ==========================================


bool ConfigValidator::isValidHostname(const string &hostname)
{
    if (hostname.empty())
        return false;

    if (hostname[0] == '.' || hostname[0] == '-' ||
        hostname[hostname.length() - 1] == '.' ||
        hostname[hostname.length() - 1] == '-')
        return false;

    for (size_t i = 0; i < hostname.length() - 1; i++)
    {
        if (hostname[i] == '.' && hostname[i + 1] == '.')
            return false;
    }
    return true;
}

bool ConfigValidator::validateListen(const vector<string> &params)
{
    if (params.size() != 1)
        return false;

    string listen = params[0];

    if (listen.empty())
        return false;

    if (listen[0] == '[')
    {
        size_t closeBracket = listen.find(']');
        if (closeBracket == string::npos)
            return false;

        string ipv6 = listen.substr(1, closeBracket - 1);

        // Just IPv6: [::1]
        if (closeBracket == listen.length() - 1)
        {
            return isValidIPv6(ipv6);
        }

        // IPv6 with port: [::1]:8080
        if (closeBracket + 1 < listen.length())
        {
            if (listen[closeBracket + 1] != ':')
                return false;
            string port = listen.substr(closeBracket + 2);
            return isValidIPv6(ipv6) && isValidPort(port);
        }

        return false;
    }

    // Case 3: Check if it contains ':'
    size_t colonPos = listen.find(':');

    // Case 3a: No colon - could be just port, IPv4, or hostname
    if (colonPos == string::npos)
    {
        return isValidPort(listen) || isValidIPv4(listen) ||
               isValidHostname(listen);
    }

    // Case 3b: Contains colon - could be IPv4:port or hostname:port
    string beforeColon = listen.substr(0, colonPos);
    string afterColon = listen.substr(colonPos + 1);

    // Check if it's a valid combination
    if (isValidPort(afterColon))
    {
        return isValidIPv4(beforeColon) || isValidHostname(beforeColon);
    }

    return false;
}

bool ConfigValidator::isValidPort(const string &port)
{
    if (port.empty())
        return false;

    for (size_t i = 0; i < port.length(); i++)
    {
        if (!isdigit(port[i]))
            return false;
    }

    stringstream ss(port);
    int portNum;
    if (!(ss >> portNum) || !ss.eof())
        return false;

    return portNum >= 1 && portNum <= 65535;
}

bool ConfigValidator::isValidIPv4(const string &ip)
{
    if (ip.empty())
        return false;

    vector<string> octets;
    stringstream ss(ip);
    string octet;

    // Split by '.'
    while (getline(ss, octet, '.'))
    {
        octets.push_back(octet);
    }

    // Must have exactly 4 octets
    if (octets.size() != 4)
        return false;

    // Validate each octet
    for (size_t i = 0; i < octets.size(); i++)
    {
        if (octets[i].empty())
            return false;

        // Check all characters are digits
        for (size_t j = 0; j < octets[i].length(); j++)
        {
            if (!isdigit(octets[i][j]))
                return false;
        }

        // Convert to integer and check range
        stringstream octetSS(octets[i]);
        int num;
        if (!(octetSS >> num) || !octetSS.eof())
            return false;

        if (num < 0 || num > 255)
            return false;
    }

    return true;
}

bool ConfigValidator::isValidIPv6(const string &ip)
{
    if (ip.empty())
        return false;

    // Special cases
    if (ip == "::")
        return true; // All zeros
    if (ip == "::1")
        return true; // Loopback

    // Check for invalid characters
    for (size_t i = 0; i < ip.length(); i++)
    {
        char c = ip[i];
        if (c != ':' && !isxdigit(c))
            return false;
    }

    // Count consecutive colons (::)
    size_t doubleColonPos = ip.find("::");
    bool hasDoubleColon = (doubleColonPos != string::npos);

    // Check for multiple "::"
    if (hasDoubleColon)
    {
        size_t secondDoubleColon = ip.find("::", doubleColonPos + 2);
        if (secondDoubleColon != string::npos)
            return false; // Multiple "::" not allowed
    }

    // Split by ':'
    vector<string> groups;
    stringstream ss(ip);
    string group;

    while (getline(ss, group, ':'))
    {
        groups.push_back(group);
    }

    // Count non-empty groups
    size_t nonEmptyGroups = 0;
    for (size_t i = 0; i < groups.size(); i++)
    {
        if (!groups[i].empty())
        {
            nonEmptyGroups++;

            // Each group max 4 hex digits
            if (groups[i].length() > 4)
                return false;

            // Check all are hex digits
            for (size_t j = 0; j < groups[i].length(); j++)
            {
                if (!isxdigit(groups[i][j]))
                    return false;
            }
        }
    }

    // IPv6 has 8 groups
    // With "::" compression, can have fewer non-empty groups
    if (hasDoubleColon)
    {
        if (nonEmptyGroups > 7)
            return false;
    }
    else
    {
        if (nonEmptyGroups != 8)
            return false;
    }

    return true;
}

bool ConfigValidator::validatePath(const vector<string> &params)
{
    if (!isValidFilesystemPath(params[0]))
        return false;

    return true;
}

bool ConfigValidator::validateCgiPath(const vector<string> &params)
{
    for (size_t i = 0; i < params.size(); ++i)
    {
        if (!isValidFilesystemPath(params[i]))
            return false;
    }
    return true;
}

bool ConfigValidator::validateMethod(const vector<string> &params)
{
    if (params.empty())
        return false;

    for (size_t i = 0; i < params.size(); i++)
    {
        const string &method = params[i];
        if (method != "GET" && method != "POST" && method != "DELETE")
            return false;
    }
    return true;
}

bool ConfigValidator::validateBoolean(const vector<string> &params)
{
    if (params.size() != 1)
        return false;

    const string &value = params[0];
    return value == "on" || value == "off";
}

bool ConfigValidator::validateSize(const vector<string> &params)
{
    if (params.size() != 1)
        return false;

    const string &size = params[0];
    if (size.empty())
        return false;

    // Check if all chars except last are digits
    for (size_t i = 0; i < size.length() - 1; i++)
    {
        if (!isdigit(size[i]))
            return false;
    }

    // Last char can be digit or size suffix
    char last = size[size.length() - 1];
    if (isdigit(last))
        return true;

    return last == 'K' || last == 'k' || last == 'M' || last == 'm' ||
           last == 'G' || last == 'g';
}

bool ConfigValidator::validateServerName(const vector<string> &params)
{
    if (params.empty())
        return false;

    for (size_t i = 0; i < params.size(); i++)
    {
        const string &name = params[i];
        if (name.empty())
            return false;
        if (!isValidHostname(const_cast<string &>(name)))
            return false;
    }
    return true;
}

bool ConfigValidator::validateIndex(const vector<string> &params)
{
    if (params.empty())
        return false;

    for (size_t i = 0; i < params.size(); i++)
    {
        const string &name = params[i];

        if (name.empty())
            return false;

        // nginx index files are filenames, not paths
        if (name.find('/') != string::npos)
            return false;

        // disallow relative path tricks
        if (name == "." || name == "..")
            return false;
    }
    return true;
}

bool ConfigValidator::isValidErrorPagePath(const string &path)
{
    if (path.empty() || path[0] != '/')
        return false;
    for (size_t i = 0; i < path.length(); i++)
    {
        char c = path[i];
        if (!isalnum(c) && !strchr("/-_.", c))
            return false;
    }
    for (size_t i = 0; i < path.length() - 1; i++)
    {
        if (path[i] == '/' && path[i + 1] == '/')
            return false;
    }
    return true;
}

bool ConfigValidator::validateErrorPage(const vector<string> &params)
{
    if (params.size() < 2)
        return false;

    for (size_t i = 0; i < params.size() - 1; i++)
    {
        stringstream ss(params[i]);
        int code;
        if (!(ss >> code) || !ss.eof())
            return false;
        if (code < 300 || code > 599)
            return false;
    }
    return isValidErrorPagePath(params[params.size() - 1]);
}

bool ConfigValidator::validateReturn(const vector<string> &params)
{
    if (params.empty() || params.size() > 2)

        return false;

    if (params.size() == 1)
    {

        if (isReturnUrl(params[0]))
            return true;

        if (!isValidReturnCode(params[0]))
            return false;

        return true;
    }

    if (!isValidReturnCode(params[0]))
        return false;

    int code = atoi(params[0].c_str());
    if (isRedirectCode(code))
    {
        if (!isReturnUrl(params[1]))
            return false;
    }

    return true;
}

bool ConfigValidator::isValidReturnCode(const string &value)
{
    if (value.empty())
        return false;

    for (size_t i = 0; i < value.size(); i++)
    {
        if (!isdigit(static_cast<unsigned char>(value[i])))
            return false;
    }

    int code = atoi(value.c_str());
    if (code < 100 || code > 599)
        return false;

    return true;
}

bool ConfigValidator::isRedirectCode(int code)
{
    return code == 301 || code == 302 || code == 303 || code == 307 ||
           code == 308;
}

bool ConfigValidator::isReturnUrl(const string &value)
{
    if (value.empty())
        return false;

    if (value.substr(0, 7) == "http://" || value.substr(0, 8) == "https://" ||
        value.substr(0, 9) == "$scheme:")
        return true;

    if (value[0] == '/')
        return true;

    return false;
}

bool ConfigValidator::validateCgiExt(const vector<string> &params)
{
    if (params.empty())
        return false;

    for (size_t i = 0; i < params.size(); i++)
    {
        const string &ext = params[i];
        if (ext.empty() || ext[0] != '.')
            return false;
        for (size_t i = 1; i < ext.size(); ++i)
        {
            if (!isalnum(static_cast<unsigned char>(ext[i])))
                return false;
        }
    }
    return true;
}

bool ConfigValidator::validateUploadStore(
    const vector<string> &params)
{
    return validatePath(params);
}

bool ConfigValidator::validateTimeout(const vector<string> &params)
{
    if (params.size() != 1)
        return false;

    const string &value = params[0];

    size_t i = 0;
    while (i < value.size() && isdigit(value[i]))
        i++;

    if (i == 0)
        return false;

    long long number;
    try
    {
        number = strtoll(value.c_str(), NULL, 10);
    }
    catch (...)
    {
        return false;
    }

    if (number <= 0)
        return false;

    string unit = value.substr(i);

    if (unit == "s" || unit == "ms" || unit == "" || unit == "m" ||
        unit == "h" || unit == "d")

        return true;

    return false;
}

bool ConfigValidator::noValidation(const vector<string> &params)
{
    (void)params;
    return true;
}

bool ConfigValidator::validateLocation(const vector<string> &params)
{
    if (params.size() != 1)
        return false;

    return isValidLocationPath(params[0]);
}

bool ConfigValidator::isValidLocationPath(const string &path)
{
    if (path.empty())
        return false;

    if (path[0] != '/')
        return false;

    for (size_t i = 0; i < path.length(); i++)
    {
        char c = path[i];
        if (!isalnum(c) && !strchr("/-_.~*$", c))
        {
            return false;
        }
    }

    for (size_t i = 1; i < path.length() - 1; i++)
    {
        if (path[i] == '/' && path[i + 1] == '/')
            return false;
    }

    if (path.length() > 1 && path[path.length() - 1] == '/')
        return false;

    return true;
}

bool ConfigValidator::isAbsolutePath(const string &path)
{
    return !path.empty() && path[0] == '/';
}

bool ConfigValidator::isValidFilesystemPath(const string &path)
{
    if (!isAbsolutePath(path))
        return false;

    for (size_t i = 0; i < path.length() - 1; i++)
    {
        if (path[i] == '/' && path[i + 1] == '/')
            return false;
    }

    return true;
}

bool ConfigValidator::validateInternal(const vector<string> &params)
{
    return params.empty();
}
