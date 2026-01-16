/*
routes the parsed request to the correct action based on the configuration.
handles static file serving (GET),
file uploads (POST),
file deletion (DELETE),
directory listings (Autoindex),
and hands off dynamic requests to the CgiHandler.
*/