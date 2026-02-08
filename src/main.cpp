#include "webserv.hpp"
#include <iostream>
#include <string>

void testDefaultValues(const ServerConfigue& server, const Location& location) {
    cout << "========================================" << endl;
    cout << "Testing Default Values Implementation" << endl;
    cout << "========================================" << endl << endl;

    // Server-level directives
    cout << "=== Server Configuration ===" << endl;
    cout << "IP:                    " << server.getHost() << endl;
    cout << "Port:                  " << server.getPort() << endl;
    
    //TODO : Server names (str list)

    // Location-level directives
    cout << "=== Location / Configuration ===" << endl;

    // String directives
    string root = location["root"];
    cout << "Root:                  " << root << " (expected: \"html\" or \"/usr/share/nginx/html\")" << endl;

    string clientMaxBodySize = location["client_max_body_size"];
    cout << "Client Max Body Size:  " << (clientMaxBodySize.empty() ? "(empty)" : clientMaxBodySize) 
         << " (expected: \"1m\")" << endl;

    string clientBodyTimeout = location["client_body_timeout"];
    cout << "Client Body Timeout:   " << (clientBodyTimeout.empty() ? "(empty)" : clientBodyTimeout) 
         << " (expected: \"60s\")" << endl;

    string uploadStore = location["upload_store"];
    cout << "Upload Store:          " << (uploadStore.empty() ? "(empty - expected)" : uploadStore) << endl;

    string cgiPath = location["cgi_path"];
    cout << "CGI Path:              " << (cgiPath.empty() ? "(empty - expected)" : cgiPath) << endl;

    string cgiTimeout = location["cgi_timeout"];
    cout << "CGI Timeout:           " << (cgiTimeout.empty() ? "(empty - expected)" : cgiTimeout) << endl;

    cout << endl;

    // List directives
    cout << "Index:                 ";
    vector<string> indexFiles = location["index"];
    if (indexFiles.empty()) {
        cout << "(empty)";
    } else {
        for (size_t i = 0; i < indexFiles.size(); ++i) {
            cout << indexFiles[i];
            if (i < indexFiles.size() - 1) cout << ", ";
        }
    }
    cout << " (expected: [\"index.html\"])" << endl;

    cout << "Allowed Methods:       ";
    vector<string> methods = location["method"];
    if (methods.empty()) {
        cout << "(empty - ALL methods allowed - expected)";
    } else {
        for (size_t i = 0; i < methods.size(); ++i) {
            cout << methods[i];
            if (i < methods.size() - 1) cout << ", ";
        }
    }
    cout << endl;

    cout << "CGI Extensions:        ";
    vector<string> cgiExts = location["cgi_ext"];
    if (cgiExts.empty()) {
        cout << "(empty - expected)";
    } else {
        for (size_t i = 0; i < cgiExts.size(); ++i) {
            cout << cgiExts[i];
            if (i < cgiExts.size() - 1) cout << ", ";
        }
    }
    cout << endl;

    cout << endl;

    // Boolean directives
    bool autoindex = location["autoindex"];
    cout << "Autoindex:             " << (autoindex ? "on" : "off") 
         << " (expected: off)" << endl;

    bool internal = location["internal"];
    cout << "Internal:              " << (internal ? "on" : "off") 
         << " (expected: off)" << endl;

    cout << endl;

    // Pair directives
    cout << "Error Pages:           ";
    pair<vector<int>, string> errorPage = location["error_page"];
    if (errorPage.first.empty()) {
        cout << "(none - expected)";
    } else {
        cout << "[";
        for (size_t i = 0; i < errorPage.first.size(); ++i) {
            cout << errorPage.first[i];
            if (i < errorPage.first.size() - 1) cout << ", ";
        }
        cout << "] -> " << errorPage.second;
    }
    cout << endl;

    cout << "Return:                ";
    pair<int, string> returnDirective = location["return"];
    if (returnDirective.first == 0 && returnDirective.second.empty()) {
        cout << "(none - expected)";
    } else {
        cout << returnDirective.first << " " << returnDirective.second;
    }
    cout << endl;

    cout << endl;
    cout << "========================================" << endl;
}
int main(int argc, char **argv)
{
    if (argc > 2)
    {
        cerr << "Usage: " << argv[0] << " [config_file]" << endl;
        return 1;
    }

    string configFile;
    if (argc == 2)
        configFile = argv[1];
    else
        configFile = "Configs/default.conf";
    try
    {
        Config config(configFile);
        config.load();
        
        config.printAST(); //? print the AST for debugging

        vector<ServerConfigue> serverConfs = config.getServerConfigues(); // each serverconfigue has a unique ip port , still need to handel server_names


        ConfigRouter router(serverConfs[0]); // a router creates a trie tree to help me map any path for each configue

        Location location = router.route("/"); // this return a location , wich allows us easy access to data + deafult handling and fallling to server

        string root = location["root"];
        cout << "Root: " << root << endl;
        pair<vector<int>, string> errorPage = location["error_page"];
        if (!errorPage.first.empty()) {
            cout << "Error Pages: [";
            for (size_t i = 0; i < errorPage.first.size(); ++i) {
                cout << errorPage.first[i];
                if (i < errorPage.first.size() - 1) cout << ", ";
            }
            cout << "] -> " << errorPage.second << endl;
        } else {
            cout << "Error Pages: (none)" << endl;
        }
        vector<string> indexFiles = location["index"];
        cout << "Index Files: ";
        if (indexFiles.empty()) {
            cout << "(none)";        } else {
            for (size_t i = 0; i < indexFiles.size(); ++i) {
                cout << indexFiles[i];
                if (i < indexFiles.size() - 1) cout << ", ";
            }
        }
        cout << endl;

        // this is an example for how to use the router
        // in this case we have an empthy file to test all the deafault that should be handled based on nginx
        // testDefaultValues(serverConfs[0], location);
        // cout << serverConfs[0].getHost() << ":" << serverConfs[0].getPort() << endl;
    }
    catch (const ConfigException &e)
    {
        cerr << "Configuration Error: " << e.what() << endl;
        return 1;
    }
    catch (const exception &e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}

