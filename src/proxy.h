#ifndef PROXY_H
#define PROXY_H

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

// Structured data for HTTP requests - This is your architectural upgrade
struct HttpRequest {
    std::string method;
    std::string host;
    std::string path;
    int port;
    bool is_https;
};

// Global variable declaration
extern std::set<std::string> blocked_domains;

// Function Prototypes
void load_blocked_domains();
bool is_blocked(const std::string& host);
void handle_tunnel(int client_sock, int server_sock);
void handle_client(int client_sock);

#endif
