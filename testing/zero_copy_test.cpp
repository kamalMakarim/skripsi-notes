#include "crow_all.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Metrics tracking
struct Metrics {
    std::string method;
    long long duration_us;
    size_t file_size;
    double throughput_mbps;
};

std::vector<Metrics> all_metrics;

// 1. Traditional Copy Method (Baseline)
std::string read_file_traditional(const std::string& filepath) {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::ifstream file(filepath, std::ios::binary);
    if (!file) return "";
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    auto end = std::chrono::high_resolution_clock::now();
    long long duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    Metrics m;
    m.method = "Traditional Copy";
    m.duration_us = duration;
    m.file_size = content.size();
    m.throughput_mbps = (content.size() / 1024.0 / 1024.0) / (duration / 1000000.0);
    all_metrics.push_back(m);
    
    return content;
}

// 2. Memory-Mapped File (mmap)
std::string read_file_mmap(const std::string& filepath) {
    auto start = std::chrono::high_resolution_clock::now();
    
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd < 0) return "";
    
    struct stat sb;
    if (fstat(fd, &sb) < 0) {
        close(fd);
        return "";
    }
    
    void* mapped = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        close(fd);
        return "";
    }
    
    // Advise kernel about access pattern
    madvise(mapped, sb.st_size, MADV_SEQUENTIAL);
    
    std::string content(static_cast<char*>(mapped), sb.st_size);
    
    munmap(mapped, sb.st_size);
    close(fd);
    
    auto end = std::chrono::high_resolution_clock::now();
    long long duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    Metrics m;
    m.method = "mmap";
    m.duration_us = duration;
    m.file_size = content.size();
    m.throughput_mbps = (content.size() / 1024.0 / 1024.0) / (duration / 1000000.0);
    all_metrics.push_back(m);
    
    return content;
}

// 3. mmap with MADV_WILLNEED (prefetch hint)
std::string read_file_mmap_willneed(const std::string& filepath) {
    auto start = std::chrono::high_resolution_clock::now();
    
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd < 0) return "";
    
    struct stat sb;
    if (fstat(fd, &sb) < 0) {
        close(fd);
        return "";
    }
    
    void* mapped = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        close(fd);
        return "";
    }
    
    // Prefetch the entire file
    madvise(mapped, sb.st_size, MADV_WILLNEED);
    madvise(mapped, sb.st_size, MADV_SEQUENTIAL);
    
    std::string content(static_cast<char*>(mapped), sb.st_size);
    
    munmap(mapped, sb.st_size);
    close(fd);
    
    auto end = std::chrono::high_resolution_clock::now();
    long long duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    Metrics m;
    m.method = "mmap+WILLNEED";
    m.duration_us = duration;
    m.file_size = content.size();
    m.throughput_mbps = (content.size() / 1024.0 / 1024.0) / (duration / 1000000.0);
    all_metrics.push_back(m);
    
    return content;
}

// 4. Buffered read with larger buffer
std::string read_file_buffered(const std::string& filepath) {
    auto start = std::chrono::high_resolution_clock::now();
    
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd < 0) return "";
    
    struct stat sb;
    if (fstat(fd, &sb) < 0) {
        close(fd);
        return "";
    }
    
    std::string content;
    content.resize(sb.st_size);
    
    const size_t BUFFER_SIZE = 1024 * 1024; // 1MB buffer
    size_t total_read = 0;
    
    while (total_read < sb.st_size) {
        ssize_t bytes_read = read(fd, &content[total_read], 
                                  std::min(BUFFER_SIZE, sb.st_size - total_read));
        if (bytes_read <= 0) break;
        total_read += bytes_read;
    }
    
    close(fd);
    content.resize(total_read);
    
    auto end = std::chrono::high_resolution_clock::now();
    long long duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    Metrics m;
    m.method = "Buffered 1MB";
    m.duration_us = duration;
    m.file_size = content.size();
    m.throughput_mbps = (content.size() / 1024.0 / 1024.0) / (duration / 1000000.0);
    all_metrics.push_back(m);
    
    return content;
}

// 5. Direct I/O (O_DIRECT) - bypass page cache
std::string read_file_direct(const std::string& filepath) {
    auto start = std::chrono::high_resolution_clock::now();
    
    int fd = open(filepath.c_str(), O_RDONLY | O_DIRECT);
    if (fd < 0) {
        // O_DIRECT might fail, fallback to regular
        fd = open(filepath.c_str(), O_RDONLY);
        if (fd < 0) return "";
    }
    
    struct stat sb;
    if (fstat(fd, &sb) < 0) {
        close(fd);
        return "";
    }
    
    // O_DIRECT requires aligned buffers
    const size_t ALIGNMENT = 4096;
    const size_t aligned_size = ((sb.st_size + ALIGNMENT - 1) / ALIGNMENT) * ALIGNMENT;
    
    void* buffer;
    if (posix_memalign(&buffer, ALIGNMENT, aligned_size) != 0) {
        close(fd);
        return "";
    }
    
    ssize_t bytes_read = read(fd, buffer, aligned_size);
    
    std::string content;
    if (bytes_read > 0) {
        content.assign(static_cast<char*>(buffer), std::min((size_t)bytes_read, (size_t)sb.st_size));
    }
    
    free(buffer);
    close(fd);
    
    auto end = std::chrono::high_resolution_clock::now();
    long long duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    Metrics m;
    m.method = "Direct I/O";
    m.duration_us = duration;
    m.file_size = content.size();
    m.throughput_mbps = (content.size() / 1024.0 / 1024.0) / (duration / 1000000.0);
    all_metrics.push_back(m);
    
    return content;
}

// Helper function to create test file
void create_test_file(const std::string& filename, size_t size_mb) {
    std::ofstream file(filename, std::ios::binary);
    const size_t CHUNK_SIZE = 1024 * 1024; // 1MB chunks
    std::vector<char> buffer(CHUNK_SIZE, 'A');
    
    for (size_t i = 0; i < size_mb; i++) {
        file.write(buffer.data(), CHUNK_SIZE);
    }
    
    std::cout << "Created test file: " << filename << " (" << size_mb << " MB)\n";
}

// Print metrics
void print_metrics() {
    std::cout << "\n========== PERFORMANCE METRICS ==========\n";
    std::cout << "Method              | Time (ms) | Throughput (MB/s)\n";
    std::cout << "--------------------|-----------|-----------------\n";
    
    for (const auto& m : all_metrics) {
        printf("%-19s | %9.2f | %15.2f\n", 
               m.method.c_str(), 
               m.duration_us / 1000.0, 
               m.throughput_mbps);
    }
    std::cout << "=========================================\n\n";
}

int main() {
    crow::SimpleApp app;
    
    const std::string test_file = "test_file.bin";
    const size_t FILE_SIZE_MB = 10; // 10MB test file
    
    // Create test file if it doesn't exist
    struct stat buffer;
    if (stat(test_file.c_str(), &buffer) != 0) {
        create_test_file(test_file, FILE_SIZE_MB);
    }
    
    // Route 1: Traditional copy
    CROW_ROUTE(app, "/traditional")
    ([&test_file](){
        auto content = read_file_traditional(test_file);
        auto resp = crow::response(content);
        resp.set_header("Content-Type", "application/octet-stream");
        return resp;
    });
    
    // Route 2: mmap
    CROW_ROUTE(app, "/mmap")
    ([&test_file](){
        auto content = read_file_mmap(test_file);
        auto resp = crow::response(content);
        resp.set_header("Content-Type", "application/octet-stream");
        return resp;
    });
    
    // Route 3: mmap with WILLNEED
    CROW_ROUTE(app, "/mmap-willneed")
    ([&test_file](){
        auto content = read_file_mmap_willneed(test_file);
        auto resp = crow::response(content);
        resp.set_header("Content-Type", "application/octet-stream");
        return resp;
    });
    
    // Route 4: Buffered read
    CROW_ROUTE(app, "/buffered")
    ([&test_file](){
        auto content = read_file_buffered(test_file);
        auto resp = crow::response(content);
        resp.set_header("Content-Type", "application/octet-stream");
        return resp;
    });
    
    // Route 5: Direct I/O
    CROW_ROUTE(app, "/direct")
    ([&test_file](){
        auto content = read_file_direct(test_file);
        auto resp = crow::response(content);
        resp.set_header("Content-Type", "application/octet-stream");
        return resp;
    });
    
    // Metrics endpoint
    CROW_ROUTE(app, "/metrics")
    ([](){
        return "Check console for metrics";
    });
    
    // Info endpoint
    CROW_ROUTE(app, "/")
    ([](){
        return R"(
Zero-Copy File Server Test Suite
=================================

Available endpoints:
- /traditional     : Standard ifstream read
- /mmap           : Memory-mapped file
- /mmap-willneed  : mmap with prefetch hint
- /buffered       : Buffered read (1MB chunks)
- /direct         : Direct I/O (O_DIRECT)

Test with: curl http://localhost:18080/<endpoint> -o /dev/null

Then check console for performance metrics.
        )";
    });
    
    std::cout << "Starting Zero-Copy Test Server on port 18080\n";
    std::cout << "Test endpoints are ready. Use curl or browser to test.\n";
    std::cout << "Example: curl http://localhost:18080/mmap -o /dev/null\n\n";
    
    // Print metrics periodically
    std::thread metrics_thread([]{
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            if (!all_metrics.empty()) {
                print_metrics();
                all_metrics.clear();
            }
        }
    });
    metrics_thread.detach();
    
    app.port(18080).multithreaded().run();
    
    return 0;
}