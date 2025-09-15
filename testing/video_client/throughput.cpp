#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>

#define BUF_SIZE 8192

// 🔧 Change these to match your server
const std::string SERVER_IP = "192.168.105.60";  // <-- put your server VM IP here
const int SERVER_PORT = 8080;                  // <-- server port

int main() {
    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP.c_str(), &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        return 1;
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        return 1;
    }

    // Send HTTP GET request
    std::string request =
        "GET / HTTP/1.1\r\n"
        "Host: " + SERVER_IP + "\r\n"
        "Connection: close\r\n\r\n";

    if (send(sockfd, request.c_str(), request.size(), 0) < 0) {
        perror("send");
        close(sockfd);
        return 1;
    }

    char buffer[BUF_SIZE];
    ssize_t n;
    size_t total_bytes = 0;

    auto start = std::chrono::high_resolution_clock::now();

    while ((n = read(sockfd, buffer, BUF_SIZE)) > 0) {
        total_bytes += n;
    }

    auto end = std::chrono::high_resolution_clock::now();
    close(sockfd);

    std::chrono::duration<double> elapsed = end - start;
    double seconds = elapsed.count();
    double throughput_mbps = (total_bytes * 8.0) / (seconds * 1e6);  // bits → Mbps

    std::cout << "Server: " << SERVER_IP << ":" << SERVER_PORT << "\n";
    std::cout << "Downloaded: " << total_bytes / (1024.0 * 1024.0) << " MB\n";
    std::cout << "Time: " << seconds << " sec\n";
    std::cout << "Throughput: " << throughput_mbps << " Mbps\n";

    return 0;
}
