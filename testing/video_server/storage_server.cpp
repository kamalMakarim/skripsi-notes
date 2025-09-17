#include <iostream>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main() {
    const int port = 8080;
    const char* videoPath = "videos/sample1.mp4";

    // Open file from storage
    int fd = open(videoPath, O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }
    struct stat st{};
    fstat(fd, &st);
    size_t fileSize = st.st_size;

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

    std::cout << "Storage server: http://localhost:" << port << "/video.mp4\n";

    while (true) {
        int client = accept(server, nullptr, nullptr);
        if (client < 0) continue;

        std::string header =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: video/mp4\r\n"
            "Content-Length: " + std::to_string(fileSize) + "\r\n"
            "Connection: close\r\n\r\n";
        send(client, header.c_str(), header.size(), 0);

        off_t offset = 0;
        while (offset < fileSize) {
            ssize_t sent = sendfile(client, fd, &offset, fileSize - offset);
            if (sent <= 0) break;
        }

        close(client);
    }

    close(fd);
}
