#include "proxy.h"

std::set<std::string> blocked_domains;

void load_blocked_domains() {
    std::ifstream file("config/blocked_domains.txt");
    std::string domain;
    blocked_domains.clear();
    while (file >> domain) {
        blocked_domains.insert(domain);
    }
}

bool is_blocked(const std::string& host) {
    if (host.empty()) return false;
    return blocked_domains.find(host) != blocked_domains.end();
}

void handle_tunnel(int client_sock, int server_sock) {
    fd_set fds;
    char buffer[BUFFER_SIZE];
    while (true) {
        FD_ZERO(&fds);
        FD_SET(client_sock, &fds);
        FD_SET(server_sock, &fds);
        int max_fd = std::max(client_sock, server_sock);
        if (select(max_fd + 1, &fds, NULL, NULL, NULL) < 0) break;

        if (FD_ISSET(client_sock, &fds)) {
            int len = recv(client_sock, buffer, sizeof(buffer), 0);
            if (len <= 0) break;
            send(server_sock, buffer, len, 0);
        }
        if (FD_ISSET(server_sock, &fds)) {
            int len = recv(server_sock, buffer, sizeof(buffer), 0);
            if (len <= 0) break;
            send(client_sock, buffer, len, 0);
        }
    }
}

void handle_client(int client_sock) {
    char buffer[BUFFER_SIZE];
    int bytes_read = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) { 
        close(client_sock); 
        return; 
    }
    buffer[bytes_read] = '\0';

    HttpRequest req;
    std::string request_str(buffer);
    std::stringstream ss(request_str);
    ss >> req.method >> req.path;

    if (req.method == "CONNECT") {
        req.is_https = true;
        size_t colon_pos = req.path.find(':');
        if (colon_pos != std::string::npos) {
            req.host = req.path.substr(0, colon_pos);
            req.port = std::stoi(req.path.substr(colon_pos + 1));
        } else {
            req.host = req.path;
            req.port = 443;
        }
    } else {
        req.is_https = false;
        size_t host_pos = request_str.find("Host: ");
        if (host_pos != std::string::npos) {
            size_t host_end = request_str.find("\r\n", host_pos);
            req.host = request_str.substr(host_pos + 6, host_end - (host_pos + 6));
            req.port = 80;
        } else {
            req.host = "";
            req.port = 80;
        }
    }

    if (!req.host.empty() && is_blocked(req.host)) {
        std::cout << "[BLOCKED] " << req.host << std::endl;
        std::string response = "HTTP/1.1 403 Forbidden\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nAccess Denied by Proxy: This site is blocked.";
        send(client_sock, response.c_str(), response.length(), 0);
        close(client_sock);
        return;
    }

    struct hostent* server = gethostbyname(req.host.c_str());
    if (server == NULL) { 
        close(client_sock); 
        return; 
    }

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(req.port);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(server_sock); 
        close(client_sock); 
        return;
    }

    if (req.is_https) {
        std::string ok_resp = "HTTP/1.1 200 Connection Established\r\n\r\n";
        send(client_sock, ok_resp.c_str(), ok_resp.length(), 0);
        handle_tunnel(client_sock, server_sock);
    } else {
        send(server_sock, buffer, bytes_read, 0);
        char remote_buffer[BUFFER_SIZE];
        int remote_bytes;
        while ((remote_bytes = recv(server_sock, remote_buffer, sizeof(remote_buffer), 0)) > 0) {
            send(client_sock, remote_buffer, remote_bytes, 0);
        }
    }

    close(server_sock);
    close(client_sock);
}
