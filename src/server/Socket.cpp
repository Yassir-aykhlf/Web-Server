/*
A utility class wrapping low-level C system calls (socket, bind, listen, accept, fcntl).
It ensures sockets are correctly configured as non-blocking and reusable (SO_REUSEADDR).
*/