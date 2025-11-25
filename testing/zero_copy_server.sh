#!/bin/bash

# Zero-Copy File Server Test Script
# ===================================

echo "Zero-Copy File Server Test Suite"
echo "================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Build the server
echo -e "${BLUE}[1/4] Building the server...${NC}"
g++ -std=c++14 zero_copy_test.cpp -o zero_copy_server -lpthread -O3
if [ $? -ne 0 ]; then
    echo "Build failed! Make sure you have:"
    echo "  - crow_all.h in the same directory"
    echo "  - g++ with C++14 support"
    echo "  - pthread library"
    exit 1
fi
echo -e "${GREEN}Build successful!${NC}"
echo ""

# Start the server in background
echo -e "${BLUE}[2/4] Starting server...${NC}"
./zero_copy_server &
SERVER_PID=$!
sleep 2

# Check if server started
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "Server failed to start!"
    exit 1
fi
echo -e "${GREEN}Server started (PID: $SERVER_PID)${NC}"
echo ""

# Run tests
echo -e "${BLUE}[3/4] Running performance tests...${NC}"
echo "Testing each method 3 times for consistency..."
echo ""

METHODS=("traditional" "mmap" "mmap-willneed" "buffered" "direct")

for method in "${METHODS[@]}"; do
    echo "Testing: $method"
    for i in {1..3}; do
        curl -s "http://localhost:18080/$method" -o /dev/null
        sleep 0.5
    done
    echo ""
done

echo -e "${GREEN}Tests complete!${NC}"
echo ""

# Wait a bit for metrics to be printed
echo -e "${BLUE}[4/4] Waiting for metrics...${NC}"
sleep 6

# Cleanup
echo ""
echo "Stopping server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo ""
echo "Test complete! Check the output above for performance metrics."
echo ""
echo "Additional testing options:"
echo "  - Modify FILE_SIZE_MB in the code for larger tests"
echo "  - Use 'ab' (Apache Bench) for concurrent testing:"
echo "    ab -n 100 -c 10 http://localhost:18080/mmap"
echo "  - Use 'wrk' for more advanced benchmarking:"
echo "    wrk -t4 -c100 -d30s http://localhost:18080/mmap"