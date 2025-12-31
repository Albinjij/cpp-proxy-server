#include "proxy.h"

// Define the global variable here
std::set<std::string> blocked_domains;

void load_blocked_domains() {
    std::ifstream file("config/blocked_domains.txt");
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) blocked_domains.insert(line);
    }
}

bool is_blocked(const std::string& host) {
    for (const auto& domain : blocked_domains) {
        if (host.find(domain) != std::string::npos) return true;
    }
    return false;
}

void handle_tunnel(int client_sock, int server_sock) {
    fd_set read_fds;
    int max_fd = (client_sock > server_sock) ? client_sock : server_sock;

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(client_sock, &read_fds);
        FD_SET(server_sock, &read_fds);

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) break;

        char buffer[BUFFER_SIZE];
        
        if (FD_ISSET(client_sock, &read_fds)) {
            int bytes = recv(client_sock, buffer, BUFFER_SIZE, 0);
            if (bytes <= 0) break;
            send(server_sock, buffer, bytes, 0);
        }

        if (FD_ISSET(server_sock, &read_fds)) {
            int bytes = recv(server_sock, buffer, BUFFER_SIZE, 0);
            if (bytes <= 0) break;
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

    if (method == "CONNECT") {
        port = 443;
        size_t colon = url.find(':');
        host = (colon != std::string::npos) ? url.substr(0, colon) : url;
        if (colon != std::string::npos) port = std::stoi(url.substr(colon + 1));
    } else {
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

    if (is_blocked(host)) {
        std::string msg = "HTTP/1.1 403 Forbidden\r\n\r\nBlocked by Proxy.";
        send(client_sock, msg.c_str(), msg.length(), 0);
        std::cout << "BLOCKED: " << host << std::endl;
        close(client_sock);
        return;
    }

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
        std::string response = "HTTP/1.1 200 Connection Established\r\n\r\n";
        send(client_sock, response.c_str(), response.length(), 0);
        handle_tunnel(client_sock, server_sock);
    } else {
        send(server_sock, request.c_str(), request.length(), 0);
        while ((bytes_read = recv(server_sock, buffer, BUFFER_SIZE, 0)) > 0) {
            send(client_sock, buffer, bytes_read, 0);
        }
    }

    close(server_sock);
    close(client_sock);
}
