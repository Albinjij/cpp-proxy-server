# C++ Multithreaded HTTP & HTTPS Proxy Server
**Developed by Albin Jiji**

## Project Overview
* It acts as an intermediary system that accepts client network requests.
* It handles TCP connections and parses HTTP requests.
* It relays server responses back to the clients and forwards traffic to destination servers.

## Features
* **TCP Socket Communication:** Built from scratch using standard POSIX socket libraries.
* **Concurrency:** Implements a thread-per-connection model to handle multiple clients simultaneously.
* **HTTPS Support (Extension):** Implements the `CONNECT` method to establish a TCP connection and tunnel encrypted traffic.
* **Domain Filtering:** Implements a rule-based blacklist mechanism to block specific domains via a configuration file.

## Project Structure
* **src/proxy.h**: Global header containing protocol constants, structure definitions, and function prototypes.
* **src/main.cpp**: The server entry point; handles socket setup and thread management.
* **src/proxy_logic.cpp**: Contains the implementation for request parsing, domain filtering, and data relaying.
* **config/blocked_domains.txt**: Filtering rule file listing blacklisted domains.
* **DESIGN.md**: Detailed documentation on architecture and logical layers.
* **Makefile**: Optimized build script for multi-file compilation and linking.
* **test_script.sh**: Automated test suite for verifying functionality.

## How to Compile
Build the project using the Makefile
```bash
make
```
## How to Run
Start the server (default listening on Port 8888):
```bash
./proxy_server
```
## How to Test
You can use the automated test script or manual commands to demonstrate usage

### 1. Automated Test Suite
**Note:** Ensure the script has execution permissions before running.
```bash
chmod +x test_script.sh
./test_script.sh
```
### 2. Manual HTTPS Tunneling Test
Verify that the proxy correctly handles secure `CONNECT` requests by tunneling the encrypted traffic
```bash
curl -v -x localhost:8888 https://www.google.com
```
### 3. Manual Blocked Site Test
Confirm that blacklisted domains listed in your configuration file are correctly filtered and blocked

```bash
curl -v -x localhost:8888 http://www.facebook.com
```
**Expected Output:** `Access Denied by Proxy: This site is blocked.`

---
## Proof of Functionality

### 1. Server Initialization
![Server Running](image_proofs/server_running.png)

### 2. HTTP Request Success
![HTTP Success](image_proofs/http_request_success.png)

### 3. HTTPS Tunneling (CONNECT method)
![HTTPS Success](image_proofs/https_connect_success.png)

### 4. Domain Filtering (403 Forbidden)
![Blocked Domain](image_proofs/blocked_domain_test.png)

### 5. Proxy Activity Logs
![Proxy Logs](image_proofs/logs_output.png)

### 6. Automated Test Suite Results
![Automated Tests](image_proofs/automated_tests.png)
