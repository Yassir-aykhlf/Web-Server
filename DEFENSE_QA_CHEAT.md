# Webserv Defense Q&A Cheat Sheet

## Core Architecture

**Q: What is your server model?**  
A: It’s a single-process, non-blocking, event-driven server using `poll()`. One main loop watches listener sockets, client sockets, and CGI pipes.

**Q: Why `poll()` (or equivalent)?**  
A: It handles many connections concurrently without one thread/process per client, and monitors read/write readiness in the same loop.

**Q: Where is multiplexing done?**  
A: In the main event loop (`EventLoop::run`) with one `poll()` call per iteration.

## Read/Write Semantics

**Q: How do you avoid blocking?**  
A: All sockets and CGI pipes are non-blocking; we only read/write when `poll()` marks the FD ready.

**Q: One read/write per client per loop?**  
A: Yes—dispatch is event-based, with one logical read or write step per ready event, then state transition.

**Q: What happens on `recv/send` failure?**  
A: On socket I/O failure, the client is closed and removed from poll tracking.

**Q: Do you check `errno` after socket I/O?**  
A: No, not in this project path.

## HTTP Handling

**Q: How do you know a request is complete?**  
A: The parser state machine handles request line, headers, and body (`Content-Length`/chunked), then emits complete or error.

**Q: Unknown methods?**  
A: Handled safely without crash and mapped to an error response.

**Q: Keep-alive vs close?**  
A: We honor connection headers, reset client state for next request on keep-alive, or close after full response.

## Configuration

**Q: Multiple servers/ports supported?**  
A: Yes. Each configured listener is registered in the poll set.

**Q: Same port conflict?**  
A: Bind fails cleanly; invalid listener setup does not continue.

**Q: Hostname routing?**  
A: Request is routed using `Host`/`server_name` matching after header parsing.

## Routing and Files

**Q: Static file and directory behavior?**  
A: Location/root rules resolve paths; files are served directly, directories use index files or autoindex.

**Q: Upload flow?**  
A: POST stores file in upload directory; GET returns it; DELETE removes it with correct status codes.

## CGI

**Q: How is CGI integrated?**  
A: CGI runs in a child process with non-blocking pipes; parent polls pipes and builds final HTTP response.

**Q: CGI methods supported?**  
A: GET and POST are supported on CGI routes.

**Q: CGI error/timeout behavior?**  
A: Script failure returns 5xx, long-running scripts are bounded by timeout (e.g., 504), and server stays alive.

## Stress and Stability

**Q: How do you validate robustness?**  
A: With `siege` under concurrency, checking availability, failed transactions, and memory trend (RSS).

**Q: Hanging connections?**  
A: Connection lifecycle is explicit (reading/writing/cleanup), and poll list is kept consistent on removal.

## Short Speaking Templates

- Our invariant is: only operate on ready FDs, keep states explicit, and remove invalid clients immediately.
- Parser decides completeness, router selects location, handler builds response, and event loop orchestrates I/O.
- On abnormal paths, we prefer deterministic cleanup and explicit HTTP errors instead of undefined behavior.
