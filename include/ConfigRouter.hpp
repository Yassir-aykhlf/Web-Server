#pragma once

#include "ConfigNode.hpp"
#include "ConfigExceptions.hpp"
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
  ConfigNode server;

public:
  Location route(URI path); // this will return a location in a server
                                 // maybe a should creat a class location , that will be returned
                                 // and the data inside it will be accesed like this with operator[] overload (Location obj["Directive"]) the data will be template maybe
                                 // if data is not defined get server data or use default if not in server
};

// TODO :
// First : i need to understand what is happening in the reqeust/respons multiplexer ...

// ============================================================
//! STEP 1 — HttpConfig
// Pick the right server from the config
// Based on: client IP, port, and Host header
// Most specific match wins
// If nothing matches → use first server in config
// ============================================================

// ============================================================
//! STEP 2 — ConfigRouter
// Takes the picked server
// Finds the right location based on the request path
// Longest matching path wins
// Falls back to / if nothing matches
// Returns 404 if / doesn't exist either
// ============================================================

// ============================================================
//! STEP 3 — route(path)
// Takes the request path (e.g. /upload/images/photo.jpg)
// Loops through all locations in the server
// Finds the best match
// Returns a Location object
// ============================================================

// ============================================================
//! STEP 4 — Location
// Holds the matched location config
// Gives access to directives with operator[]
// If directive not in location → check server
// If not in server either → return default
// ============================================================