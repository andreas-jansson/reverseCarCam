#ifndef RPI3TEST_OBD2_H
#define RPI3TEST_OBD2_H


void sendUdpMessage(const char* address, int port, const std::string& message);

void listenForUdpMessages(void* arg);



#endif //RPI3TEST_OBD2_H
