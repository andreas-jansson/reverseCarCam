#ifndef RPI3TEST_CANDATAHANDLER_H
#define RPI3TEST_CANDATAHANDLER_H

#include <cstdio>
#include <cstdint>
#include <mutex>

enum gearPosition{
    nodata=-1,
    reverse,
    gear1,
    gear2,
    gear3,
    gear4,
    gear5,
};

struct CanMsg{
    std::uint32_t id;
    int32_t data;
};


class CanDataHandler{

public:
    CanDataHandler() = default;
    static CanDataHandler *GetInstance();

    void insert_data(char* a_buffer, ssize_t a_len);
    std::int32_t get_wheel_position();
    gearPosition get_gear_position();
    bool get_connection_status(){return m_canActive;};
private:
    static CanDataHandler * instance;
    static std::mutex s_mutexWheelPos;
    static std::mutex s_mutexGearPos;
    std::int32_t m_wheelPos;
    bool m_canActive{};


    CanDataHandler(CanDataHandler &other) = delete;

    void operator=(const CanDataHandler &) = delete;

    void set_wheel_position(int32_t a_data);
    void set_gear_position();
    CanMsg parse_msg(char* a_buffer, ssize_t a_len);

};





#endif //RPI3TEST_CANDATAHANDLER_H
