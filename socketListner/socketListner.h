#ifndef RPI3TEST_SOCKETLISTNER_H
#define RPI3TEST_SOCKETLISTNER_H


void t_socketServer(void* arg);

bool parseMsg(char* a_buffer, ssize_t a_len);

#endif //RPI3TEST_SOCKETLISTNER_H
