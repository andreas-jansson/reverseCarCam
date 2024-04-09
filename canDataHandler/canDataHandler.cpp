#include <cstring>
#include <vector>
#include <map>
#include "canDataHandler.h"


CanDataHandler* CanDataHandler::instance{};
std::mutex CanDataHandler::s_mutexWheelPos;

CanDataHandler* CanDataHandler::GetInstance(){
    if (instance == nullptr)
    {
        instance = new CanDataHandler();
    }
    return instance;
}

void CanDataHandler::insert_data(char* a_buffer, ssize_t a_len){
    std::map< std::uint32_t, std::string> supportedCanIds;
    supportedCanIds.emplace(0xA1, "wheelPos");
    supportedCanIds.emplace(0xA3,"gearPos");

    CanMsg msg = parse_msg(a_buffer, a_len);

    if(msg.id == 0xFFFF)
        printf("Error id %d\n", msg.id);

    if(supportedCanIds.at(msg.id) == "wheelPos"){
        set_wheel_position(static_cast<int32_t>(msg.data));
    }
    else if(supportedCanIds.at(msg.id) == "gearPos"){
        set_wheel_position(static_cast<int32_t>(msg.data));

    }else{
        printf("canId %u not supported\n", msg.id);
    }



}

CanMsg CanDataHandler::parse_msg(char* a_buffer, ssize_t a_len){
    CanMsg msg{};
    //msg.data = new char[a_len]; //FIXME mem leak
    memcpy(&msg.id, a_buffer, sizeof(std::uint32_t));

    if(msg.id < 0 || msg.id > 2047)
        return CanMsg{0xFFFF,0xF};
    if(a_len > 8)
        printf("bad id 0x%x data %d len %zd\n", msg.id, msg.data, a_len);

    memcpy(&msg.data, a_buffer+sizeof(std::uint32_t), a_len - sizeof(std::uint32_t));
    return msg;
}

void CanDataHandler::set_wheel_position(int32_t a_data){
    std::lock_guard<std::mutex> lock(s_mutexWheelPos);
    m_wheelPos = a_data;
    printf("m_wheelPos: %d\n", m_wheelPos);

}
void CanDataHandler::set_gear_position(){

}

std::int32_t CanDataHandler::get_wheel_position(){
    std::lock_guard<std::mutex> lock(s_mutexWheelPos);
    return m_wheelPos;
}
