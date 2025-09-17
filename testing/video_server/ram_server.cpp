#include <iostream>
#include <vector>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    const int port = 8081;
    const char* videoPath = "videos/sample1.mp4";

    // Load file fully into RAM
    std::ifstream file(videoPath, std::ios::binary | std::ios::ate);
    if (!file) { perror("open"); return 1; }
    size_t fileSize = file.tellg();
    file.seekg(0);
    std::vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    file.close();

    // Create server
    int server = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(server, (sockaddr*)&addr, sizeof(addr));
    listen(server, 5);

    std::cout << "RAM server: http://localhost:" << port << "/video.mp4\n";

    while (true) {
        int client = accept(server, nullptr, nullptr);
        if (client < 0) continue;

        std::string header =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: video/mp4\r\n"
            "Content-Length: " + std::to_string(fileSize) + "\r\n"
            "Connection: close\r\n\r\n";
        send(client, header.c_str(), header.size(), 0);

        send(client, buffer.data(), fileSize, 0);

        close(client);
    }
}
