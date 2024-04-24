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

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket creation failed" << std::endl;
        return;
    }



    struct sockaddr_in address{};
    int port = 6666;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("192.168.1.240");
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return;
    }

    if (listen(server_fd, 3) < 0) { // Backlog set to 3
        std::cerr << "Listen failed" << std::endl;
        return;
    }
    std::cout << "Server listening on port " << port << std::endl;

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
            char buffer[256] = {0};
            ssize_t msgLen = read(new_socket, buffer, 256);

            if(msgLen <= 0)
                break;
            dataHandler->insert_data(buffer);
        }
    }
    close(new_socket);
}

