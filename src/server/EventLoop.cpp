/*
The heart of the non-blocking architecture.
It implements the Reactor pattern using poll().
It monitors all file descriptors (listeners, clients, CGI pipes)
and dispatches events (POLLIN/POLLOUT) to the appropriate read/write handlers.
*/