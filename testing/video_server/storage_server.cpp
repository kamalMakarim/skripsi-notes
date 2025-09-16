// storage_server.cpp
#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <sys/sendfile.h>

int main() {
    const int port = 8080;
    const char* videoPath = "videos/sample1.mp4";

    // Open video file
    int fd = open(videoPath, O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }

    struct stat st{};
    if (fstat(fd, &st) < 0) { perror("fstat"); close(fd); return 1; }
    size_t fileSize = st.st_size;

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

    std::cout << "Storage zero-copy server running on http://localhost:" << port << "/video.mp4\n";

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

        // Use sendfile for zero-copy transfer
        off_t offset = 0;
        while (offset < fileSize) {
            ssize_t sent = sendfile(client, fd, &offset, fileSize - offset);
            if (sent <= 0) {
                perror("sendfile");
                break;
            }
        }

        close(client);
    }

    close(fd);
    close(server);
    return 0;
}
