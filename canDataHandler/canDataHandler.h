#ifndef RPI3TEST_CANDATAHANDLER_H
#define RPI3TEST_CANDATAHANDLER_H

#include <cstdio>
#include <cstdint>
#include <mutex>
#include <map>

enum gearPosition{
    nodata=-1,
    reverse,
    gear1,
    gear2,
    gear3,
    gear4,
    gear5,
};

enum printDataFormat{
    hexFormat,
    binFormat,
    progressBarFormat,
};

struct CanMsg{
    std::uint32_t id;
    std::uint32_t size;
    std::uint8_t data[64];
};


class CanDataHandler{

public:
    CanDataHandler() = default;
    static CanDataHandler *GetInstance();

    void insert_data(char* a_buffer);
    std::int32_t get_wheel_position();
    double get_radian();
    gearPosition get_gear_position();
    void set_rpm(int a_rpmB3, int a_rpmB4);
    int get_rpm();
    bool get_connection_status(){return m_canActive;};
    static void configure(int a_maxCanId=9998,
                          int a_minCanId=0,
                          int m_wheelPosMaxValue=255,
                          int a_wheelPosMaxSpan=40,
                          printDataFormat a_format=hexFormat,
                          int a_printBinaryCanId=-1);
private:
    static CanDataHandler * instance;
    static std::mutex s_mutexWheelPos;
    static std::mutex s_mutexGearPos;
    std::int32_t m_wheelPos;
    bool m_canActive{};
    double m_radians;

    static int s_printCanbitsId;
    static int s_maxCanId;
    static int s_minCanId;
    static int s_wheelPosMaxValue;
    static int s_wheelPosMaxSpan;
    static bool s_printRawCan;
    static int s_printBinaryCanId;
    static int s_rpmValue;
    static printDataFormat s_printFormat;
    static std::map< std::uint32_t, std::string> s_supportedCanIds;
    static std::map<std::uint32_t, CanMsg> s_canPair;

    void set_wheel_position(int32_t a_data);
    void set_gear_position();
    CanMsg parse_msg(char* a_buffer);
    void print_raw_can(CanMsg a_msg);
    void print_raw_bits_can(CanMsg a_msg);
    void print_raw_bits_single_can(CanMsg a_msg);
    void print_bars_can(CanMsg a_msg);
    void print_raw_bars_single_can(CanMsg a_msg);

    CanDataHandler(CanDataHandler &other) = delete;
    void operator=(const CanDataHandler &) = delete;
};






#endif //RPI3TEST_CANDATAHANDLER_H
