# Proxy Server Design Document

## 1. High-Level Architecture
The system is a multithreaded HTTP proxy server designed to handle concurrent client requests. It consists of the following key components:
* **Listener Socket:** Accepts incoming TCP connections on port 8888.
* **Thread Manager:** Spawns a dedicated thread for each new client connection (Thread-per-connection model).
* **HTTP Parser:** Extracts the `Host`, `Port`, and `Method` from the client's HTTP request.
* **Filter Engine:** Checks the destination hostname against a loaded blacklist (`blocked_domains.txt`).
* **Forwarder:** Establishes a connection to the remote server and relays data bi-directionally.

## 2. Concurrency Model

**Model Chosen:** Thread-per-connection.
**Rationale:**
* **Simplicity:** Each client logic is isolated in its own thread context, making the code easier to reason about compared to non-blocking I/O.
* **Isolation:** If one client stalls, it does not block the main acceptor loop or other active clients.
* **Standard Library:** Utilizes C++ `std::thread` for portable threading support.

## 3. Data Flow

1. **Client Request:** Client connects to Proxy (Port 8888) and sends an HTTP GET or CONNECT request.
2. **Parsing:** The Proxy parses the request to find the target `Host`.
3. **Filtering:** The Proxy checks if `Host` is in the blacklist.
    * *If Blocked:* Returns `403 Forbidden` and closes connection.
    * *If Allowed:* Resolves DNS to an IP address.
4. **Forwarding:**
    * Proxy connects to the Target Server.
    * Proxy sends the Client's request to the Server.
    * Proxy receives the Server's response and streams it back to the Client.

## 4. Error Handling & Security

* **Malformed Requests:** Validates the request line; returns `400 Bad Request` and closes the connection if the HTTP method or version is invalid.
* **Connection Failures:** Returns `502 Bad Gateway` if the destination server is unreachable or if DNS resolution fails.
* **Access Control:** Returns `403 Forbidden` and logs the event if a domain exists in `config/blocked_domains.txt`.
* **Design Trade-offs:** Uses blocking I/O (slow servers may stall threads) and basic validation; it is not currently hardened against advanced buffer or protocol exploits.

## 5. HTTPS Tunneling (Extension)

The proxy implements support for HTTPS via the HTTP `CONNECT` method.
* **Mechanism:** When a `CONNECT` request is received (e.g., `CONNECT google.com:443`), the proxy establishes a raw TCP connection to the destination.
* **Handshake:** It responds to the client with `HTTP/1.1 200 Connection Established`.
* **Tunneling:** It then enters a transparent tunneling mode, forwarding encrypted TCP packets bi-directionally between the client and server. This ensures end-to-end encryption is maintained.
* **Limitations and Trade-offs:** * **Blocking I/O:** The tunnel uses blocking network I/O; a very slow destination server can hold a proxy thread active until the connection is closed.
    * **Security:** While the tunnel preserves end-to-end encryption, the proxy is not hardened against sophisticated buffer overflows or protocol-level exploits.

## 6. Future Improvements

To enhance maintainability and scalability, future iterations will focus on:

* **Modular Refactoring:** Organizing the source code into subdirectories (e.g., `/src/Logger`, `/src/Metrics`) to better separate core concerns.
* **Advanced Concurrency:** Implementing a **Thread Pool** to manage worker threads efficiently and handle high volumes of connections without the overhead of spawning new threads for every request.
* **Caching Engine:** Adding an **LRU (Least Recently Used) cache** to store frequent resources, reducing latency and outbound bandwidth.
* **Request Sanitization:** Enhancing the parser to perform deeper header validation to prevent sophisticated request smuggling attacks.

