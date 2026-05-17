## What It Does

- Serves web pages and files
- Handles multiple connections at once without blocking
- Supports file uploads and deletions
- Runs CGI scripts (Python, shell, etc.)
- Uses configuration files like NGINX

## Configuration

Create a `.conf` file:

```nginx
server {
    listen 8080;
    server_name localhost;
    root ./www/html;
    
    location / {
        methods GET POST;
    }
    
    location /upload {
        methods GET POST DELETE;
        upload_store ./www/uploads;
    }
}
```

## Project Structure

```
webserv/
├── src/
│   ├── config/      # Config file parsing
│   ├── server/      # Socket and event loop
│   ├── http/        # HTTP protocol handling
│   ├── cgi/         # CGI script execution
│   └── utils/       # Helper functions
├── include/         # Header files
├── config/          # Config files
├── www/             # Web content
└── Makefile
```
