#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <netdb.h>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <set>
#include <sys/select.h>

#define BUFFER_SIZE 8192
#define PORT 8888

std::set<std::string> blocked_domains;

// Load blocked domains from config
void load_blocked_domains() {
    std::ifstream file("config/blocked_domains.txt");
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) blocked_domains.insert(line);
    }
}

// Check if host is blocked
bool is_blocked(const std::string& host) {
    for (const auto& domain : blocked_domains) {
        if (host.find(domain) != std::string::npos) return true;
    }
    return false;
}

// HTTPS TUNNEL: Blindly forward bytes between client and server
void handle_tunnel(int client_sock, int server_sock) {
    fd_set read_fds;
    int max_fd = (client_sock > server_sock) ? client_sock : server_sock;

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(client_sock, &read_fds);
        FD_SET(server_sock, &read_fds);

        // Wait for data on either side
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) break;

        char buffer[BUFFER_SIZE];
        
        // Forward Client -> Server
        if (FD_ISSET(client_sock, &read_fds)) {
            int bytes = recv(client_sock, buffer, BUFFER_SIZE, 0);
            if (bytes <= 0) break; // Connection closed
            send(server_sock, buffer, bytes, 0);
        }

        // Forward Server -> Client
        if (FD_ISSET(server_sock, &read_fds)) {
            int bytes = recv(server_sock, buffer, BUFFER_SIZE, 0);
            if (bytes <= 0) break; // Connection closed
            send(client_sock, buffer, bytes, 0);
        }
    }
}

void handle_client(int client_sock) {
    char buffer[BUFFER_SIZE];
    int bytes_read = recv(client_sock, buffer, BUFFER_SIZE, 0);
    if (bytes_read <= 0) {
        close(client_sock);
        return;
    }

    std::string request(buffer, bytes_read);
    std::stringstream ss(request);
    std::string method, url;
    ss >> method >> url;

    std::string host;
    int port = 80;

    // PARSING: Handle CONNECT (HTTPS) vs GET (HTTP)
    if (method == "CONNECT") {
        port = 443;
        size_t colon = url.find(':');
        host = (colon != std::string::npos) ? url.substr(0, colon) : url;
        if (colon != std::string::npos) port = std::stoi(url.substr(colon + 1));
    } else {
        // Standard HTTP parsing
        size_t host_pos = url.find("://");
        if (host_pos != std::string::npos) url = url.substr(host_pos + 3);
        
        size_t path_pos = url.find('/');
        host = url.substr(0, path_pos);
        
        size_t port_pos = host.find(':');
        if (port_pos != std::string::npos) {
            port = std::stoi(host.substr(port_pos + 1));
            host = host.substr(0, port_pos);
        }
    }

    // FILTERING
    if (is_blocked(host)) {
        std::string msg = "HTTP/1.1 403 Forbidden\r\n\r\nBlocked by Proxy.";
        send(client_sock, msg.c_str(), msg.length(), 0);
        std::cout << "BLOCKED: " << host << std::endl;
        close(client_sock);
        return;
    }

    // CONNECT TO REMOTE SERVER
    struct hostent* host_entry = gethostbyname(host.c_str());
    if (!host_entry) { close(client_sock); return; }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    std::memcpy(&server_addr.sin_addr, host_entry->h_addr, host_entry->h_length);

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(client_sock);
        return;
    }

    std::cout << "Connected to: " << host << std::endl;

    if (method == "CONNECT") {
        // HTTPS: Send 200 OK, then Tunnel
        std::string response = "HTTP/1.1 200 Connection Established\r\n\r\n";
        send(client_sock, response.c_str(), response.length(), 0);
        handle_tunnel(client_sock, server_sock);
    } else {
        // HTTP: Forward original request, then stream response
        send(server_sock, request.c_str(), request.length(), 0);
        while ((bytes_read = recv(server_sock, buffer, BUFFER_SIZE, 0)) > 0) {
            send(client_sock, buffer, bytes_read, 0);
        }
    }

    close(server_sock);
    close(client_sock);
}

int main() {
    load_blocked_domains();
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 10);

    std::cout << "HTTPS Proxy listening on port " << PORT << "..." << std::endl;

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &len);
        std::thread(handle_client, client_sock).detach();
    }
    return 0;
}
