#ifndef RPI3TEST_GRAPHIX_H
#define RPI3TEST_GRAPHIX_H

#include <iostream>
#include <mutex>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <cassert>

//values to calculate y = kx + m
struct ParkingXYK{
    std::uint32_t x[5]{};
    std::uint32_t y[5]{};
    double k[5]{};
};

struct PixelXY{
    int x;
    int y;
};


class Drawing
{
private:
    static Drawing * instance;
    static std::mutex mutex_;
    static std::uint32_t m_screenWidthBytes;
    static std::uint32_t m_screenWidthPixels;
    static std::uint32_t m_screenHeightPixels;
    static std::uint32_t m_bytesPerPixel;
    static std::uint64_t m_screenSizeBytes;
    static int m_fontHeight;
    static int m_charMaxWidth;

    Drawing() = default;

    [[nodiscard]] static std::int64_t convertPixelToOffset(std::uint32_t a_x, std::uint32_t a_y) ;

    static bool isCoordinatesValid(std::uint32_t a_x, std::uint32_t  a_y) ;

    static void swapCoordinates(uint32_t &a_x0, uint32_t &a_y0, uint32_t &a_x1, uint32_t &a_y1) ;


public:

    Drawing(Drawing &other) = delete;

    void operator=(const Drawing &) = delete;

    static Drawing *GetInstance();

    void init(std::uint32_t a_width, std::uint32_t a_height, std::uint32_t a_bytesPerPixel);

    /**
     * @param a_x x pixel of top left corner, 0 is top left of screen
     * @param a_y y pixel of top left corner, 0 is top left of screen
     * @param a_width
     * @param a_height
     * @param a_lineThickness
     * @param a_color
     * @param a_buffer the buffer to draw onto
     */
    static void drawSquare(std::uint32_t a_x, std::uint32_t a_y, std::uint32_t a_width, std::uint32_t a_height, uint16_t a_color, std::uint32_t a_lineThickness, char* a_buffer);

    /**
     *
     * @param a_x0
     * @param a_y0
     * @param a_x1
     * @param a_y1
     * @param a_lineThickness draws downwards from x0,y0
     * @param a_color rgb565
     * @param a_buffer
     */
    static void drawLine(std::uint32_t a_x0, std::uint32_t a_y0, std::uint32_t a_x1, std::uint32_t a_y1, uint16_t a_color, std::uint32_t a_lineThickness,char* a_buffer);

    static void drawStraightLine(std::uint32_t a_x0, std::uint32_t a_y0, std::uint32_t a_x1, std::uint32_t a_y1, uint16_t a_color, std::uint32_t a_lineThickness,char* a_buffer);

    void draw4PointShape(std::uint32_t a_x0, std::uint32_t a_y0, std::uint32_t a_x1, std::uint32_t a_y1, std::uint32_t a_x2, std::uint32_t a_y2, std::uint32_t a_x3, std::uint32_t a_y3, uint16_t a_color, std::uint32_t a_lineThickness, char* a_buffer);

    static int interpolate( int from , int to , double percent );

    static void drawCurve(std::uint32_t a_x0, std::uint32_t a_y0, std::uint32_t a_x2, std::uint32_t a_y2, int a_curveHeight, uint16_t a_color, std::uint32_t a_lineThickness, char* a_buffer);

    ParkingXYK calculateParkingLinesNew(std::uint32_t a_x0, std::uint32_t a_y0, std::uint32_t a_x2, std::uint32_t a_y2, int a_curveHeight, int a_nrOfLines);

    void drawParkinglinesNew(int a_curveHeight, double a_angle,uint16_t a_color, std::uint32_t a_lineThickness, char* a_buffer);

    void drawString(unsigned char* a_buffer,  std::uint32_t a_color, int a_size, int a_x, int a_y, std::string a_str);

    void drawChar(unsigned char* a_buffer,std::uint32_t a_color, int a_size, int a_x, int a_y, char a_char);

    PixelXY rotate_xy(int a_x0, int a_y0, int a_x1, int a_y1, double a_radians, bool a_isLeft);

};



#endif //RPI3TEST_GRAPHIX_H
