#include <cstring>
#include <vector>
#include <map>
#include <bitset>
#include <iostream>
#include "canDataHandler.h"


CanDataHandler* CanDataHandler::instance{};
std::mutex CanDataHandler::s_mutexWheelPos;
int CanDataHandler::s_printCanbitsId;
int CanDataHandler::s_maxCanId;
int CanDataHandler::s_minCanId;
int CanDataHandler::s_wheelPosMaxValue;
int CanDataHandler::s_wheelPosMaxSpan;
int CanDataHandler::s_printBinaryCanId;
int CanDataHandler::s_rpmValue;
bool CanDataHandler::s_printRawCan;
printDataFormat CanDataHandler::s_printFormat;

std::map< std::uint32_t, std::string> CanDataHandler::s_supportedCanIds;
std::map< std::uint32_t, CanMsg> CanDataHandler::s_canPair;


void CanDataHandler::print_raw_can(CanMsg a_msg){
    s_canPair[a_msg.id] = a_msg;

    for(const auto &i : s_canPair){
        printf("id: %2d len: %d data: ", i.second.id, i.second.size);
        for(int j=0;j<i.second.size;j++){
            printf("%2x ", i.second.data[j]);
            if(j>8)
                break;
        }
        printf("\n");
    }
    printf("\x1b[%dA\r", s_canPair.size());
    fflush(stdout);
}

void CanDataHandler::print_raw_bits_single_can(CanMsg a_msg){

    if(a_msg.id != s_printCanbitsId)
        return;

    printf("\x1b[A\r");
    printf("id: %2d len: %d data: ", a_msg.id, a_msg.size);
    for(int j=0;j<a_msg.size;j++){
        std::bitset<8> bits(a_msg.data[j]);
        std::cout<<bits<<" ";
    }
    printf("\n");
    //printf("\x1b[A\r");
    fflush(stdout);
}

void CanDataHandler::print_raw_bars_single_can(CanMsg a_msg){

    if(a_msg.id != s_printCanbitsId)
        return;

    char progress[] = "\u2588";
    //char progress = '.';

    //clear rows
    for (int i = 0; i < a_msg.size; i++) {
        printf("id: %2d len: %d data[%d]: ", a_msg.id, a_msg.size, i);
        int size = 100;
        for (int j = 0; j < size; j++) {
            printf(" ", progress);
        }
        printf("\n");

    }
    printf("\n");
    fflush(stdout);
    printf("\x1b[%dA\r", a_msg.size+1);


    for (int i = 0; i < a_msg.size; i++) {
        printf("id: %2d len: %d data[%d]: ", a_msg.id, a_msg.size, i);
        int size = a_msg.data[i] / 10;
        for (int j = 0; j < size; j++) {
            printf("%s", progress);
        }
        printf("\n");

    }
    printf("\n");
    fflush(stdout);
    printf("\x1b[%dA\r", a_msg.size+1);

}

void CanDataHandler::print_raw_bits_can(CanMsg a_msg){
    s_canPair[a_msg.id] = a_msg;
    for(const auto &i : s_canPair){
        printf("id: %2d len: %d data: ", i.second.id, i.second.size);
        for(int j=0;j<i.second.size;j++){
            printf("%2x ", i.second.data[j]);
            std::bitset<8> bits(i.second.data[j]);
            std::cout<<bits<<" ";
            if(j>8)
                break;
        }
        printf("\n");
    }
    printf("\x1b[%dA\r", s_canPair.size());
    fflush(stdout);
}

void CanDataHandler::print_bars_can(CanMsg a_msg){
    s_canPair[a_msg.id] = a_msg;

    //char progress = 221;
    char progress[] = "\u2588";
    int nrRows{};
    for(const auto &msg : s_canPair) {
        for (int i = 0; i < msg.second.size; i++) {
            printf("id: %2d len: %d data[%d]: ", i, msg.second.id, msg.second.size);
            for (int j = 0; j < msg.second.data[j]; j++) {
                printf("%s", progress);
            }
            printf("\n");
            nrRows++;
            fflush(stdout);
        }
        printf("\n");
        nrRows++;
    }
    printf("\x1b[%dA\r", s_canPair.size() + nrRows);

}


CanDataHandler* CanDataHandler::GetInstance(){
    if (instance == nullptr)
    {
        s_supportedCanIds.emplace(688, "wheelPos");
        s_supportedCanIds.emplace(790, "rpmValue");
        configure();
        instance = new CanDataHandler();
    }
    return instance;
}

void CanDataHandler::insert_data(char* a_buffer){

    CanMsg msg = parse_msg(a_buffer);
    if(msg.id > s_maxCanId || msg.id < s_minCanId)
        return;

    auto it = s_supportedCanIds.find(msg.id);
    if (it != s_supportedCanIds.end()) {
        if (it->second == "wheelPos") {
            set_wheel_position(static_cast<int32_t>(msg.data[1]));
        }
        else if (it->second == "rpmValue") {
            set_rpm(static_cast<int32_t>(msg.data[2]), static_cast<int32_t>(msg.data[3]));
        }
    }
    print_raw_bits_single_can(msg);
    //print_raw_bars_single_can(msg);
    print_raw_can(msg);

    /*
    if(s_printBinaryCanId != -1){
        printf("s_printBinaryCanId: %d\n", s_printBinaryCanId);
        print_raw_bits_single_can(msg);
    }
    if(s_printFormat == hexFormat){
        printf("s_printFormat: %d\n", s_printBinaryCanId);
        print_raw_can(msg);
    }
    else if(s_printFormat == binFormat){
        printf("s_printFormat: %d\n", s_printBinaryCanId);
        print_raw_bits_can(msg);
    }
    else if(s_printFormat == progressBarFormat){
        printf("s_printFormat: %d\n", s_printBinaryCanId);
        print_bars_can(msg);
    }
    else
        printf("else s_printFormat: %d\n", s_printBinaryCanId);
        */
}

CanMsg CanDataHandler::parse_msg(char* a_buffer){
    CanMsg msg{};
    memcpy(&msg.id, a_buffer, sizeof(std::uint32_t));
    memcpy(&msg.size, (a_buffer+sizeof(std::uint32_t)), sizeof(std::uint32_t));

    if(msg.size > 0 && msg.size < 64)
        memcpy(msg.data, (a_buffer+sizeof(std::uint32_t)+sizeof(std::uint32_t)), msg.size);

    return msg;
}

void CanDataHandler::set_wheel_position(int32_t a_data){

    std::lock_guard<std::mutex> lock(s_mutexWheelPos);
    //20 - 0/255 - 235 real data
    //-20 - 0 - 20 reformatted data
//    if(a_data >= (s_wheelPosMaxValue - s_wheelPosMaxSpan))
//        m_wheelPos = s_wheelPosMaxValue - a_data;
//    else
//        m_wheelPos = a_data * -1;
    //printf("setting wheelpos %d\n", m_wheelPos);f
    if(a_data >= (s_wheelPosMaxValue - s_wheelPosMaxSpan))
        m_wheelPos =  a_data - s_wheelPosMaxValue;
    else
        m_wheelPos = a_data;

}
void CanDataHandler::set_gear_position(){

}

void CanDataHandler::set_rpm(int a_rpmB3, int a_rpmB4){

    s_rpmValue = ((a_rpmB4 * 256) + a_rpmB3) / 6.4;
}

int CanDataHandler::get_rpm(){
    return s_rpmValue;
}

std::int32_t CanDataHandler::get_wheel_position(){
    std::lock_guard<std::mutex> lock(s_mutexWheelPos);
    return m_wheelPos;
}

void CanDataHandler::configure(int a_maxCanId, int a_minCanId, int a_wheelMaxValue, int a_wheelMaxSpan, printDataFormat a_format, int a_printBinaryCanId){
    s_maxCanId = a_maxCanId;
    s_minCanId = a_minCanId;
    s_wheelPosMaxValue = a_wheelMaxValue;
    s_wheelPosMaxSpan = a_wheelMaxSpan;
    s_printFormat = a_format;
    s_printCanbitsId = a_printBinaryCanId;
    s_rpmValue = 0;
}
