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
#include "obd2.h"

void sendUdpMessage(const char* address, int port, const std::string& message) {
    using namespace std::chrono_literals;
    int sockfd;
    struct sockaddr_in servaddr;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return;
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Fill server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = inet_addr(address);
    servaddr.sin_port = htons(port);

    while(1) {
        // Send message to server
        sendto(sockfd, message.c_str(), message.length(), MSG_CONFIRM, (const struct sockaddr *) &servaddr,
               sizeof(servaddr));
            std::this_thread::sleep_for(200ms);
    }
    close(sockfd);
}

void listenForUdpMessages(void* arg) {
    int port = 35000;
    int sockfd;
    struct sockaddr_in servaddr{}, cliaddr{};

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return;
    }

    int broadcastEnable=1;
    int ret=setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

    if (ret) {
        std::cerr << "Could not enable broadcast" << std::endl;
        close(sockfd);
        return;
    }

    // Fill server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        close(sockfd);
        return;
    }

    while (true) {
        char buffer[1024];
        unsigned int len = sizeof(cliaddr);

        // Receive message from client
        int n = recvfrom(sockfd, (char *)buffer, 1024, MSG_WAITALL, (struct sockaddr *) &cliaddr, &len);
        buffer[n] = '\0';

        std::cout << "Received: " << buffer << std::endl;
    }

    close(sockfd);
}

