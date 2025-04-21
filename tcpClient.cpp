#include <iostream>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstdlib>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }
    int port = std::atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        std::cerr << "Invalid port number" << std::endl;
        return 1;
    }
    int client_socket;
    struct sockaddr_in server_addr;
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port); 
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return 1;
    }
    std::cout << "Connected to server on port " << port << "..." << std::endl;

    char buffer[1024];
    int bytes_received;
    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        std::cout << "Received: " << buffer << std::endl;
    }

    if (bytes_received == 0) {
        std::cout << "Connection closed by server" << std::endl;
    } else {
        std::cerr << "Error receiving data" << std::endl;
    }

    close(client_socket);
    return 0;
}

