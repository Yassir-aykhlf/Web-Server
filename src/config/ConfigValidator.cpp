#include "ConfigValidator.hpp"

ConfigValidator::ConfigValidator()
{
  initializeRules();
}

void ConfigValidator::initializeRules()
{
  // Method 1: Using temporary vectors
  std::vector<std::string> empty;
  std::vector<std::string> httpParents;
  httpParents.push_back("http");

  std::vector<std::string> serverParents;
  serverParents.push_back("server");

  std::vector<std::string> serverLocationParents;
  serverLocationParents.push_back("server");
  serverLocationParents.push_back("location");

  std::vector<std::string> locationParents;
  locationParents.push_back("location");

  // Block directives (can contain other directives)
  rules_["http"] = DirectiveRule(0, 0, true, httpParents, &ConfigValidator::noValidation);
  rules_["server"] = DirectiveRule(0, 0, true, httpParents, &ConfigValidator::noValidation);
  rules_["location"] = DirectiveRule(1, 1, true, serverParents, &ConfigValidator::noValidation);

  // Server context directives
  rules_["listen"] = DirectiveRule(1, 1, false, serverParents, &ConfigValidator::validateListen);
  rules_["server_name"] = DirectiveRule(1, MAX_PARAMS_SIZE, false, serverParents, &ConfigValidator::validateServerName);
  rules_["root"] = DirectiveRule(1, 1, false, serverLocationParents, &ConfigValidator::validatePath);
  rules_["index"] = DirectiveRule(1, MAX_PARAMS_SIZE, false, serverLocationParents, &ConfigValidator::validateIndex);
  rules_["error_page"] = DirectiveRule(2, MAX_PARAMS_SIZE, false, serverLocationParents, &ConfigValidator::validateErrorPage);
  rules_["client_max_body_size"] = DirectiveRule(1, 1, false, serverLocationParents, &ConfigValidator::validateSize);
  rules_["autoindex"] = DirectiveRule(1, 1, false, serverLocationParents, &ConfigValidator::validateBoolean);

  // Location context directives
  rules_["method"] = DirectiveRule(1, MAX_PARAMS_SIZE, false, locationParents, &ConfigValidator::validateMethod);
  rules_["return"] = DirectiveRule(1, 2, false, serverLocationParents, &ConfigValidator::validateReturn);
  rules_["alias"] = DirectiveRule(1, 1, false, locationParents, &ConfigValidator::validateAlias);

  // CGI directives
  rules_["cgi_ext"] = DirectiveRule(1, MAX_PARAMS_SIZE, false, locationParents, &ConfigValidator::validateCgiExt);
  rules_["cgi_path"] = DirectiveRule(1, 1, false, locationParents, &ConfigValidator::validateCgiPath);

  // Upload directives
  rules_["upload_store"] = DirectiveRule(1, 1, false, locationParents, &ConfigValidator::validateUploadStore);

  // Timeout directives
  rules_["client_body_timeout"] = DirectiveRule(1, 1, false, serverLocationParents, &ConfigValidator::validateTimeout);
  rules_["send_timeout"] = DirectiveRule(1, 1, false, serverLocationParents, &ConfigValidator::validateTimeout);
}

bool ConfigValidator::isAllowedInContext(const std::string &directive, const std::string &context)
{
  std::map<std::string, DirectiveRule>::const_iterator ruleIt = rules_.find(directive);
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

void ConfigValidator::validateNode(const ConfigNode &node, const std::string &context)
{
  std::map<std::string, DirectiveRule>::const_iterator ruleIt = rules_.find(node.getName());
  if (ruleIt == rules_.end())
    throw ConfigException("Unknown directive: " + node.getName());

  const DirectiveRule &rule = ruleIt->second;

  if (!isAllowedInContext(node.getName(), context))
  {
    throw ConfigException("Directive '" + node.getName() + "' not allowed in context '" + context + "'");
  }

  size_t paramCount = node.getArguments().size();

  if (paramCount < rule.minParams || paramCount > rule.maxParams)
    throw ConfigException("Directive '" + node.getName() + "unexpected number of parameters");

  // rule.paramValidator(node.getArguments());

  if (!rule.allowsChildren && !node.getChildren().empty())
    throw ConfigException("Directive '" + node.getName() + "' does not allow child directives");

  for (size_t i = 0; i < node.getChildren().size(); ++i)
    validateNode(node.getChildren()[i], node.getName());
}

void ConfigValidator::validate(const ConfigNode &root)
{
  validateNode(root, "http");
}

// 1. Port validator (1-65535)
bool ConfigValidator::validateListen(const std::vector<std::string> &params)
{
  if (params.size() != 1)
    return false;

  std::stringstream ss(params[0]);
  int port;
  if (!(ss >> port) || !ss.eof())
    return false;

  return port >= 1 && port <= 65535;
}

// 2. Path validator (must be absolute path or exist)
bool ConfigValidator::validatePath(const std::vector<std::string> &params)
{
  if (params.size() != 1)
    return false;

  const std::string &path = params[0];
  if (path.empty() || path[0] != '/')
    return false;

  // Optional: Check if path exists
  // struct stat info;
  // return stat(path.c_str(), &info) == 0;

  return true;
}

// 3. HTTP Method validator (GET, POST, DELETE)
bool ConfigValidator::validateMethod(const std::vector<std::string> &params)
{
  if (params.empty())
    return false;

  for (size_t i = 0; i < params.size(); i++)
  {
    const std::string &method = params[i];
    if (method != "GET" && method != "POST" && method != "DELETE")
      return false;
  }
  return true;
}

// 4. Boolean validator (on/off, true/false, yes/no)
bool ConfigValidator::validateBoolean(const std::vector<std::string> &params)
{
  if (params.size() != 1)
    return false;

  const std::string &value = params[0];
  return value == "on" || value == "off" ||
         value == "true" || value == "false" ||
         value == "yes" || value == "no";
}

// 5. Size validator (e.g., 1M, 500K, 1G)
bool ConfigValidator::validateSize(const std::vector<std::string> &params)
{
  if (params.size() != 1)
    return false;

  const std::string &size = params[0];
  if (size.empty())
    return false;

  // Check if all chars except last are digits
  for (size_t i = 0; i < size.length() - 1; i++)
  {
    if (!std::isdigit(size[i]))
      return false;
  }

  // Last char can be digit or size suffix
  char last = size[size.length() - 1];
  if (std::isdigit(last))
    return true;

  return last == 'K' || last == 'k' ||
         last == 'M' || last == 'm' ||
         last == 'G' || last == 'g';
}

// 6. Server name validator (domain names)
bool ConfigValidator::validateServerName(const std::vector<std::string> &params)
{
  if (params.empty())
    return false;

  for (size_t i = 0; i < params.size(); i++)
  {
    const std::string &name = params[i];
    if (name.empty())
      return false;

    // Basic validation: alphanumeric, dots, hyphens, underscores
    for (size_t j = 0; j < name.length(); j++)
    {
      char c = name[j];
      if (!std::isalnum(c) && c != '.' && c != '-' && c != '_')
        return false;
    }
  }
  return true;
}

// 7. Index files validator
bool ConfigValidator::validateIndex(const std::vector<std::string> &params)
{
  if (params.empty())
    return false;

  for (size_t i = 0; i < params.size(); i++)
  {
    if (params[i].empty())
      return false;
  }
  return true;
}

// 8. Error page validator (error_code path, e.g., "404 /404.html")
bool ConfigValidator::validateErrorPage(const std::vector<std::string> &params)
{
  if (params.size() < 2)
    return false;

  // All params except last should be error codes
  for (size_t i = 0; i < params.size() - 1; i++)
  {
    std::stringstream ss(params[i]);
    int code;
    if (!(ss >> code) || !ss.eof())
      return false;

    if (code < 300 || code > 599)
      return false;
  }

  // Last param should be a path
  const std::string &path = params[params.size() - 1];
  return !path.empty() && path[0] == '/';
}

// 9. Return/Redirect validator (code [url])
bool ConfigValidator::validateReturn(const std::vector<std::string> &params)
{
  if (params.size() < 1 || params.size() > 2)
    return false;

  std::stringstream ss(params[0]);
  int code;
  if (!(ss >> code) || !ss.eof())
    return false;

  // Valid HTTP redirect codes
  if ((code < 300 || code > 308) && code != 200 && code != 201 && code != 204)
    return false;

  // If URL is provided, basic validation
  if (params.size() == 2)
  {
    const std::string &url = params[1];
    if (url.empty())
      return false;
  }

  return true;
}

// 10. CGI extension validator (.py, .php, .pl)
bool ConfigValidator::validateCgiExt(const std::vector<std::string> &params)
{
  if (params.empty())
    return false;

  for (size_t i = 0; i < params.size(); i++)
  {
    const std::string &ext = params[i];
    if (ext.empty() || ext[0] != '.')
      return false;
  }
  return true;
}

// 11. CGI path validator
bool ConfigValidator::validateCgiPath(const std::vector<std::string> &params)
{
  if (params.size() != 1)
    return false;

  const std::string &path = params[0];
  return !path.empty() && path[0] == '/';
}

// 12. Upload store validator
bool ConfigValidator::validateUploadStore(const std::vector<std::string> &params)
{
  return validatePath(params);
}

// 13. Alias validator
bool ConfigValidator::validateAlias(const std::vector<std::string> &params)
{
  return validatePath(params);
}

// 14. Timeout validator (in seconds)
bool ConfigValidator::validateTimeout(const std::vector<std::string> &params)
{
  if (params.size() != 1)
    return false;

  std::stringstream ss(params[0]);
  int timeout;
  if (!(ss >> timeout) || !ss.eof())
    return false;

  return timeout > 0 && timeout <= 3600; // Max 1 hour
}

// 15. No validation (always valid)
bool ConfigValidator::noValidation(const std::vector<std::string> &params)
{
  (void)params;
  return true;
}