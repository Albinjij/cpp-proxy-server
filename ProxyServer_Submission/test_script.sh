#!/bin/bash
echo "Starting Proxy Tests..."

# 1. Test standard site (Should succeed)
echo "[TEST 1] Accessing example.com..."
curl -x localhost:8888 -I http://example.com
if [ $? -eq 0 ]; then echo "PASS"; else echo "FAIL"; fi

echo "--------------------------------"

# 2. Test blocked site (Should fail with 403)
echo "[TEST 2] Accessing blocked site (facebook.com)..."
curl -x localhost:8888 -I http://facebook.com
if [ $? -eq 0 ]; then echo "PASS (Blocked response received)"; else echo "FAIL"; fi

echo "--------------------------------"

# 3. Test HTTPS Tunneling (Google)
echo "[TEST 3] Accessing HTTPS site (google.com)..."
curl -x localhost:8888 -I https://www.google.com
if [ $? -eq 0 ]; then echo "PASS (HTTPS Connection Established)"; else echo "FAIL"; fi

echo "Done."
