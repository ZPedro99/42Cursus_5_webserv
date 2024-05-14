# Basics of HTTP Server

HTTP (Hypertext Transfer Protocol) is the cornerstone protocol used for exchanging information over the internet, serving as the foundation of the World Wide Web. It enables communication between web browsers and servers, facilitating both server-side and client-side programming.

## Request-Response Cycle

The request-response cycle is a critical process in web communication, involving:

1. **Client Request:** A client (e.g., web browser or mobile app) sends a request to the server, which includes the requested resource and any additional parameters.
2. **Server Processing:** Upon receiving the request, the server processes it and generates a response message.
3. **Server Response:** The server sends a response to the client, typically including the requested resource along with any additional information or metadata.
4. **Client Processing:** The client receives and processes the server's response, often rendering the content in a web browser or displaying it in an app.

Clients may initiate additional requests, repeating this cycle as necessary.

## Creating a Valid HTTP Request

To create a valid HTTP request, the following elements are essential:

- **URL:** Represents the unique name pointing to a specific resource on the server.
- **HTTP Method:** Indicates the action the client desires the server to take, such as `GET`, `POST`, or `DELETE`.
- **Headers:** Provide context and additional instructions to the server from the client.
- **Body (optional):** Contains data sent from the client to the server as part of the request.

Example of a simple GET request:
```
GET /watch?v=dQw4w9WgXcQ HTTP/1.1
```

Example of a more detailed GET request:
```
GET /api/data HTTP/1.1
Host: example.com
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/89.0.4389.82 Safari/537.36
Accept: application/json
Accept-Language: en-US,en;q=0.5
Authorization: Token abc123
Cache-Control: no-cache
Connection: keep-alive
Referer: https://www.google.com/
Pragma: no-cache
```

## Response

The server's response to an HTTP request includes:

- **Status Line:** Contains the HTTP version, status code, and a message indicating the outcome of the request.
- **Headers:** Provide additional information about the response.
- **Message Body:** Contains the actual response data (e.g., HTML, JSON, XML).

Example response:
```
HTTP/1.1 200 OK
Date: Sun, 28 Mar 2023 10:15:00 GMT
Content-Type: application/json
Server: Apache/2.4.39 (Unix) OpenSSL/1.1.1c PHP/7.3.6
Content-Length: 1024

{
	"name": "John Wick",
	"email": "johnwick@example.com",
	"age": 35,
	"address": {
		"street": "123 Main St",
		"city": "Anytown",
		"state": "CA",
		"zip": "12345"
	}
}
```

## Common HTTP Status Codes

- `100 Continue`
- `101 Switching Protocols`
- `200 OK`
- `201 Created`
- `202 Accepted`
- `203 Non-Authoritative Information`
- `404 Not Found` - The requested resource was not found on the server.
- `500 Internal Server Error` - The server encountered an error while processing the request.
- `301 Moved Permanently` - The requested resource has been permanently moved to a new URL.


----

# I/O Multiplexing

I/O Multiplexing is a technique used for managing multiple input/output operations over a single blocking system call. It's crucial for applications that need to handle multiple data streams simultaneously without dedicating a separate thread or process to each one, thus significantly improving efficiency and performance in networked applications.

## `select()`

The `select()` system call allows a program to monitor multiple file descriptors to see if one or more of them are ready for an I/O operation (e.g., reading or writing).

**How `select()` Works:**

1. Prepare three sets of file descriptors: read, write, and exceptions.
2. Define a timeout duration.
3. Invoke `select()`, which blocks until at least one descriptor becomes ready or a timeout happens.
4. After `select()` returns, check which descriptors are ready and proceed with the necessary operations.

**Limitations:**

- Has a fixed limit on the number of descriptors (`FD_SETSIZE`).
- Inefficient for large sets of descriptors due to its linear scanning mechanism.

## `poll()`

`poll()` serves a similar purpose to `select()`, monitoring multiple file descriptors, but it does so using an array of `struct pollfd` structures to overcome some of `select()`'s limitations.

**How `poll()` Works:**

1. Initialize an array of `pollfd` structures with descriptors and the events of interest.
2. Call `poll()`, specifying the array, its size, and a timeout.
3. After returning, iterate through the array to identify which descriptors had their events occur and handle them accordingly.

Unlike `select()`, `poll()` is more scalable and does not have a preset limit on the number of descriptors it can handle.

## `epoll()`

Exclusive to Linux, `epoll()` is a modern alternative to `select()` and `poll()`, designed to efficiently handle a large number of file descriptors.

**How `epoll()` Works:**

1. Create an `epoll` instance with `epoll_create()`.
2. Use `epoll_ctl()` to add file descriptors to the `epoll` instance, specifying the events to monitor.
3. Call `epoll_wait()`, which blocks until events occur, returning only those descriptors with active events.

**Advantages:**

- Reduces CPU usage by eliminating the need to check all file descriptors.
- Scales efficiently, capable of managing thousands of concurrent connections.

In conclusion, while `select()` and `poll()` offer portability across Unix-like systems, `epoll()` provides superior performance and scalability on Linux systems, especially suitable for server applications that handle many simultaneous connections.

We use our epoll group functions in our `startServers()`. We create an `epoll_fd` variable that is the control structure for all the I/O operations that will be managed by epoll.
The _numberOfSv is the number of servers our cluster will have, and it is used as a parameter to determine the number of file descriptors that we need to handle.

We now create a structure `epoll_event` with a buffer of 10, that is initialized to monitor both input (EPOLLIN) and output (EPOLLOUT) events. This will decide what type of activities on the sockets will be monitored - read/write.

`epoll_ctl(epoll_fd, EPOLL_CTL_ADD, events.data.fd, &events)` register the server's sockets and allows the server cluster to track events on these sockets.

`epoll_wait(epoll_fd, event_buffer, 10, 5000)` is called to wait for events on the monitored sockets. The function blocks until events are available, or the timeout (5000 milliseconds) occurs. If epoll_wait returns a negative value, it checks if a signal (gSignalStatus) has interrupted the wait. If the signal indicates a negative scenario, it simply returns from the method.

For each event detected, we check if the file descriptor from the event buffer belongs to a known server socket (indicating an incoming connection attempt). If it's a server socket, it accepts the new incoming connection using `accept(event_buffer[i].data.fd, (sockaddr*)&client_address, (socklen_t*)&addrlen)`, assigning it to a new client socket. The new client socket is then set to non-blocking mode and added to the epoll monitoring initServer using `epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event_buffer[i]) < 0)`.

If an existing connection has new data to read (EPOLLIN event), the connection is handled by a method called connectionHandler that sets the connection and sends the request. If `checkTimeout()` closes a fd we break the cycle and go back to the beginning, so as not to iterate over possible removed FDs from buffer.  might be called to perform periodic maintenance, such as removing inactive sockets from the epoll.



## Usefull links:

<details>
<summary>Introductory Videos</summary>

- **How to create Web Server in C++ under 600s - Tutorial** [#cpp](https://www.youtube.com/hashtag/cpp) [#programming](https://www.youtube.com/hashtag/programming)
  [YouTube Link](https://www.youtube.com/watch?v=VlUO6ERf1TQ&ab_channel=ProgrammerEgg)
- **Web Server Concepts and Examples**
  [YouTube Link](https://www.youtube.com/watch?v=9J1nJOivdyw)
</details>

<details>
<summary>Advanced Videos</summary>

- Playlist web server c++
  [YouTube Playlist Link](https://www.youtube.com/playlist?list=PLPUbh_UILtZW1spXkwszDBN9YhtB0tFKC)
- **Select() in C**
  [YouTube Link](https://www.youtube.com/watch?v=Y6pFtgRdUts)
- **C++ Web Server from Scratch | Part 1: Creating a Socket Object**
  [YouTube Link](https://www.youtube.com/watch?v=YwHErWJIh6Y&ab_channel=EricOMeehan)
- **Creating a TCP Server in C++ [Linux / Code Blocks]**
  [YouTube Link](https://www.youtube.com/watch?v=cNdlrbZSkyQ&ab_channel=SloanKelly)
</details>

<details>
<summary>Blog Posts</summary>

- [Building a simple HTTP server from scratch](https://medium.com/from-the-scratch/http-server-what-do-you-need-to-know-to-build-a-simple-http-server-from-scratch-d1ef8945e4fa)
- [Building an HTTP server from scratch in C++](https://medium.com/@osasazamegbe/showing-building-an-http-server-from-scratch-in-c-2da7c0db6cb7)
- [Building a simple server with C++](https://ncona.com/2019/04/building-a-simple-server-with-cpp/)
</details>

<details>
<summary>GitHub Repositories</summary>

- [Webserv_42](https://github.com/Kaydooo/Webserv_42?tab=readme-ov-file)
- [http_server](https://github.com/Dungyichao/http_server?tab=readme-ov-file)
</details>

<details>
<summary>RFCs</summary>

- [How to read RFC](https://www.ietf.org/blog/how-read-rfc/)
- [RFC 7230](https://www.rfc-editor.org/rfc/pdfrfc/rfc7230.txt.pdf)
- [RFC 7231](https://www.rfc-editor.org/rfc/pdfrfc/rfc7231.txt.pdf)
- [RFC 7232](https://www.rfc-editor.org/rfc/pdfrfc/rfc7232.txt.pdf)
- [RFC 7233](https://www.rfc-editor.org/rfc/pdfrfc/rfc7233.txt.pdf)
- [RFC 7234](https://www.rfc-editor.org/rfc/pdfrfc/rfc7234.txt.pdf)
- [RFC 7235](https://www.rfc-editor.org/rfc/pdfrfc/rfc7235.txt.pdf)
- **Curso de Redes - O que é um RFC**
  [YouTube Link](https://www.youtube.com/watch?v=HnqplkfzdTE)
</details>

<details>
<summary>CGI</summary>

- [What is Common Gateway Interface (CGI)](https://stackoverflow.com/questions/2089271/what-is-common-gateway-interface-cgi)
- [RFC 3875](https://www.ietf.org/rfc/rfc3875.txt)
- [O que é CGI e qual é sua finalidade](https://pt.stackoverflow.com/questions/93308/o-que-%C3%A9-cgi-e-qual-%C3%A9-sua-finalidade)
- [CGI Scripts](https://opensource.com/article/17/12/cgi-scripts)
</details>

<details>
<summary>HTTP</summary>

- [HTTP Developer Guide](https://developer.mozilla.org/en-US/docs/Web/HTTP)
- **HTTP Requests on a C Server**
  [YouTube Link](https://www.youtube.com/watch?v=3p5dL_oPjeA&ab_channel=EricOMeehan)
- [HTTP Request Methods](https://www.w3schools.com/tags/ref_httpmethods.asp)
- [HTTP Server: Everything you need to know to Build a simple HTTP server from scratch](https://medium.com/from-the-scratch/http-server-what-do-you-need-to-know-to-build-a-simple-http-server-from-scratch-d1ef8945e4fa)
- **HTTP vs HTML: Unveiling Network Protocols using Telnet**
  [YouTube Link](https://www.youtube.com/watch?v=ArXMa111x7A)
- **How to test or send http request using telnet**
  [YouTube Link](https://www.youtube.com/watch?v=ufWUVA9DpLk)
- [Visual nuts and bolts of a web server](https://medium.com/@md.abir1203/visual-nuts-and-bots-of-a-webserver-282e6034e169)
- [HTTP Requests and Responses](https://medium.com/@gwenilorac/http-requests-and-responses-cc989147fd8c)
- [Responding to HTTP requests](https://flylib.com/books/en/2.407.1/responding_to_http_requests.html)
- [Great HTTP Explanation](https://www.garshol.priv.no/download/text/http-tut.html)
</details>

<details>
<summary>Response Content Type</summary>

- [Content Type](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Type)
</details>

<details>
<summary>Select and Poll Functions</summary>

- [Multiple Socket Connections: FDSET and Select in Linux](https://www.binarytides.com/multiple-socket-connections-fdset-select-linux/)
- [Using Poll Instead of Select](https://www.ibm.com/docs/en/i/7.1?topic=designs-using-poll-instead-select)
- [Chapter 6. I/O Multiplexing: The select and poll Functions](https://notes.shichao.io/unp/ch6/)
</details>

<details>
<summary>Other</summary>

- [web.notaduo.com/notes/29srs/edit](https://web.notaduo.com/notes/29srs/edit)
</details>
