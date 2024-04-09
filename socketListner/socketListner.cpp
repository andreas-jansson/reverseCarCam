#include <iostream>
#include <cstring>      // For memset
#include <unistd.h>     // For close()
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include "socketListner.h"
#include "../canDataHandler/canDataHandler.h"


void t_socketServer(void* arg){

    // Create a socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket creation failed" << std::endl;
        return;
    }

    // Define the server address
    struct sockaddr_in address;
    int port = 6666;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("192.168.1.240"); // Listen on any IP address
    address.sin_port = htons(port);

    // Bind the socket to the IP address and port
    printf("attempting to bind to ip %s\n", inet_ntoa(address.sin_addr));
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return;
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) { // Backlog set to 3
        std::cerr << "Listen failed" << std::endl;
        return;
    }
    std::cout << "Server listening on port " << port << std::endl;

    uint32_t  id{};
    int8_t  data{};
    CanDataHandler *dataHandler = CanDataHandler::GetInstance();

    int new_socket{-1};
    while(true) {
        int addrlen = sizeof(address);
        new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen);
        if (new_socket < 0) {
            std::cerr << "Accept failed" << std::endl;
            return;
        }
        while (true) {
            // Receive message from client
            char buffer[1024] = {0};
            ssize_t msgLen = read(new_socket, buffer, 1024);
            memcpy(&id, buffer, 4);
            memcpy(&data, buffer+4, 1);

            if(msgLen <= 0)
                break;
            if(msgLen <= 8)
                dataHandler->insert_data(buffer, msgLen);

                // Send response to client
            //std::string message = "Hello from server";
            //send(new_socket, message.c_str(), message.length(), 0);
            //std::cout << "Hello message sent" << std::endl;

        }
    }
    // Close the socket
    close(new_socket);
}

