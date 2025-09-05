// ram_server.cpp
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <fstream>

int main() {
    const int port = 8081;
    const char* videoPath = "videos/sample1.mp4";

    // Load file into RAM
    std::ifstream file(videoPath, std::ios::binary | std::ios::ate);
    if (!file) { std::cerr << "Failed to open file\n"; return 1; }
    size_t fileSize = file.tellg();
    file.seekg(0);

    std::vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    file.close();

    // HTTP server
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }
    listen(server, 5);

    std::cout << "RAM zero-copy server running on http://localhost:" << port << "/video.mp4\n";

    while (true) {
        int client = accept(server, nullptr, nullptr);
        if (client < 0) { perror("accept"); continue; }

        // Minimal HTTP response
        std::string header =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: video/mp4\r\n"
            "Content-Length: " + std::to_string(fileSize) + "\r\n"
            "Connection: close\r\n\r\n";

        send(client, header.c_str(), header.size(), 0);

        // Send video from RAM buffer
        send(client, buffer.data(), fileSize, 0);

        close(client);
    }

    close(server);
    return 0;
}
