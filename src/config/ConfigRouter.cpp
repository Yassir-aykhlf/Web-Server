#include "ConfigRouter.hpp"

// Creat another tree , from locations

Location ConfigRouter::route(std::string path)
{

  URI uri(path); //! sanitize path and provide functionallitys to manage it , like getNext segment and so on

  // TODO : in construction :

  //! creat a trie structure out of all locations in this serve

  //////////////////////////////////////////////////////////////////////////////////////

  // TODO: in route

  // route will find that location , and return it , soo travers the tree and return the match or unavailable
}
