/*
Encapsulates the state of a single client connection.
It tracks the connection lifecycle state,
manages read/write buffers,
holds the pending Request/Response objects,
and tracks active CGI processes for that client.
*/