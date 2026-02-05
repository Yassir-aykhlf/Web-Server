#pragma once

#include "ConfigNode.hpp"
#include "ConfigExceptions.hpp"
#include "Location.hpp"
#include "ServerConfigue.hpp"
#include "URI.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <string>

// For the router first i should define the blue print , undrestand how it does work
// so router does rout data , we have a request that will be ask us , i want to get/post/delet this (data) form this location(path)
//  then we need to have for each sever
// then learn about the trie structeur

//? before routing
// after defining the configue router bleuprint , the task should be as folows
// First : find the server that we want to use it's configs
// Seconed : creat a router instance for that server ; (this is before defining any logic for routing)
// thered :

class ConfigRouter
{
  ServerConfigue serverConf;

public:
  ConfigRouter(const ServerConfigue &serverConf) : serverConf(serverConf) {};
  Location route(std::string path);
};

// TODO :
//! soo in serverconfigue i will allerdy build a locationtrie struct , wich will be a tree that hold all location
//! struct will have (path , Location , childrens)
//! the router is the one who will use that trie to find the location that match the path
//! so the route method will take a path string , then search in the trie for the best match location
//! then return that location instance