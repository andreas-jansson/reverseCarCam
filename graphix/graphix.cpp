#include <map>
#include "graphix.h"


int Drawing::m_fontHeight;
int Drawing::m_charMaxWidth;

 Drawing* Drawing::GetInstance(){
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance == nullptr)
    {
        instance = new Drawing();
    }
    return instance;
}

void Drawing::init(std::uint32_t a_width, std::uint32_t a_height, std::uint32_t a_bytesPerPixel)
{
    m_bytesPerPixel = a_bytesPerPixel;
    m_screenWidthPixels = a_width;
    m_screenHeightPixels = a_height;
    m_screenWidthBytes = m_screenWidthPixels * m_bytesPerPixel;
    m_screenSizeBytes = m_screenWidthBytes * m_screenHeightPixels;
    m_fontHeight = 7;
    m_charMaxWidth = 5;
}

std::int64_t Drawing::convertPixelToOffset(std::uint32_t a_x, std::uint32_t a_y) {

    //align with rgb format
    std::int64_t xOffset = (m_screenWidthPixels * a_y + a_x) * m_bytesPerPixel;
    if(xOffset > m_screenSizeBytes)
        return -1;
    return xOffset;
}

int Drawing::interpolate( int from , int to , double percent )
{
    int difference = to - from;
    return from + ( difference * percent );
}

void Drawing::swapCoordinates(uint32_t &a_x0, uint32_t &a_y0, uint32_t &a_x1, uint32_t &a_y1) {
    uint32_t tmp = a_x0;
    a_x0 = a_x1;
    a_x1 = tmp;
    tmp = a_y0;
    a_y0 = a_y1;
    a_y1 = tmp;
}

bool Drawing::isCoordinatesValid(std::uint32_t a_x, std::uint32_t  a_y) {
    if(a_x > m_screenWidthPixels){
        //printf("bad value x: %d > %d \n", a_x, m_screenWidthPixels);
        return false;
    }
    if(a_y > m_screenHeightPixels){
        //printf("bad value y1: %d > %d\n", a_y, m_screenHeightPixels);
        return false;
    }
    return true;
}

ParkingXYK Drawing::calculateParkingLinesNew(std::uint32_t a_x0, std::uint32_t a_y0, std::uint32_t a_x2, std::uint32_t a_y2, int a_curveHeight, int a_nrOfLines) {

    ParkingXYK parkingXyk{};
    auto* parkLine = new double[a_nrOfLines];
    parkLine[0] = 0.2;
    parkLine[1] = 0.5;
    parkLine[2] = 0.90;

    constexpr int slopeCounterLimit{7};
    bool* isParkLineFilled = new bool[a_nrOfLines]{};
    bool* isParkSlopeFilled = new bool[a_nrOfLines]{};


    int dx = static_cast<int>(a_x2 - a_x0);
    int dy = static_cast<int>(a_y2 - a_y0);

    double len = sqrt(dx*dx+dy*dy);
    double udx = dx / len;
    double udy = dy / len;

    double px = -udy;
    double py = udx;

    int midX = static_cast<int>((a_x0 + a_x2) / 2);
    int midY = static_cast<int>((a_y0 + a_y2) / 2);
    int x1 = static_cast<int>(midX + px * a_curveHeight);
    int y1 = static_cast<int>(midY + py * a_curveHeight);

    int slopeCounter{};
    for( double i = 0 ; i < 1 ; i += 0.01 )
    {
        int xa = interpolate( static_cast<int>(a_x0) , x1 , i );
        int ya = interpolate( static_cast<int>(a_y0) , y1 , i );
        int xb = interpolate( x1 , static_cast<int>(a_x2) , i );
        int yb = interpolate( y1 , static_cast<int>(a_y2) , i );

        // The Black Dot
        int x = interpolate( xa , xb , i );
        int y = interpolate( ya , yb , i );

        //in order to draw lines within this curved box
        for(int line = 0; line < a_nrOfLines; line++) {
            if (i >= parkLine[line] && !isParkSlopeFilled[line] && slopeCounter < slopeCounterLimit) {
                if (!isParkLineFilled[line]) {
                    if(line == a_nrOfLines - 1){
                        parkingXyk.x[line] = a_x2;
                        parkingXyk.y[line] = a_y2;
                    }
                    else {
                        parkingXyk.x[line] = x;
                        parkingXyk.y[line] = y;
                    }
                    isParkLineFilled[line] = true;
                }

                if (slopeCounter == slopeCounterLimit - 1) {
                    int dx = (static_cast<int>(x) - static_cast<int>(parkingXyk.x[line]));
                    int dy = (static_cast<int>(y) - static_cast<int>(parkingXyk.y[line]));
                    auto k = static_cast<double>(dy) / static_cast<double>(dx);
                    parkingXyk.k[line] = dx != 0 ? k : 1;
                    isParkSlopeFilled[line] = true;
                    slopeCounter = 0;
                    break;
                }
                slopeCounter++;

                if (parkingXyk.x[line] > m_screenWidthBytes)
                    printf("bad x0[%d] value: %d\n",line, parkingXyk.x[line]);
                if (parkingXyk.y[line] > m_screenHeightPixels)
                    printf("bad y0[%d] value: %d\n",line, parkingXyk.y[line]);
            }
        }
    }
    return parkingXyk;
}

//######################### Public Methods ###############################

void Drawing::drawSquare(std::uint32_t a_x, std::uint32_t a_y, std::uint32_t a_width, std::uint32_t a_height, uint16_t a_color, std::uint32_t a_lineThickness, char* a_buffer) {

    // create the top/bottom line
    auto *topLine = new uint16_t[a_width];
    std::fill(topLine, topLine + a_width, a_color);

    //draw the top line
    for (int y = 0; y < a_lineThickness; y++) {
        int xOffset = static_cast<int>(((a_y + y) * m_screenWidthBytes) + a_x);
        memcpy(a_buffer + (xOffset), topLine, a_width);
    }

    //draw bottom line
    for (int y = 0; y < a_lineThickness; y++) {
        int xOffset = static_cast<int>( (((a_y+a_height) + y) * m_screenWidthBytes) + a_x );
        memcpy(a_buffer + xOffset, topLine, a_width);
    }
    delete[] topLine;

    // create the side line
    auto *sideLine = new uint16_t[a_lineThickness];
    std::fill(sideLine, sideLine + a_lineThickness, a_color);
    for (int y = 0; y < a_height; y++) {
        int xOffset = static_cast<int>( (a_y + y) * m_screenWidthBytes + a_x );
        memcpy(a_buffer + xOffset, sideLine, a_lineThickness);
        memcpy(a_buffer + xOffset + a_width, sideLine, a_lineThickness);

    }
    delete[] sideLine;
}

void Drawing::drawLine(std::uint32_t a_x0, std::uint32_t a_y0, std::uint32_t a_x1, std::uint32_t a_y1, uint16_t a_color, std::uint32_t a_lineThickness,char* a_buffer) {
    if(!isCoordinatesValid(a_x0, a_y0))
        return;
    if(!isCoordinatesValid(a_x1, a_y1))
        return;

    if(a_x0 == a_x1 || a_y0 == a_y1){
        drawStraightLine(a_x0, a_y0, a_x1, a_y1, a_color, a_lineThickness, a_buffer);
        return;
    }
    if(a_x0 > a_x1)
        swapCoordinates(a_x0, a_y0, a_x1, a_y1);

    //distance between two points: sqrt((x2-x1)^2 + (y2 - y1)^2);
    //int distance = static_cast<int>( round(sqrt(pow(static_cast<int>(a_x1 - a_x0), 2) + pow(static_cast<int>(a_y1 - a_y0), 2))) );
    int distance = abs(static_cast<int>(a_x0) - static_cast<int>(a_x1));
    int dx = static_cast<int>(a_x1) - static_cast<int>(a_x0);
    int dy = static_cast<int>(a_y1) - static_cast<int>(a_y0);
    auto k = static_cast<double>(dy) / static_cast<double>(dx);
    //printf("x0 %d x1 %d y0 %d y1 %d distance %d k %f\n", a_x0, a_x1, a_y0, a_y1, distance, k);

    uint16_t pixel = a_color;
    for (int x = 0; x < distance; x++) {
        int y = static_cast<int>(k * x);
        std::uint64_t xOffset = convertPixelToOffset((a_x0+x), (a_y0 + y));
        if(xOffset == -1){
            continue;
        }

        memcpy(a_buffer + (xOffset), &pixel, sizeof(pixel));

        for (int y1 = 0; y1 < a_lineThickness; y1++) {
            std::uint64_t xOffsetThickness = xOffset + (m_screenWidthBytes * y1);
            xOffsetThickness = xOffsetThickness % 2 == 0 ? xOffsetThickness : xOffsetThickness + 1;
            xOffsetThickness = xOffsetThickness > m_screenSizeBytes ? m_screenSizeBytes : xOffsetThickness;
            memcpy(a_buffer + (xOffsetThickness), &pixel, sizeof(pixel));
        }
    }
}

void Drawing::drawStraightLine(std::uint32_t a_x0, std::uint32_t a_y0, std::uint32_t a_x1, std::uint32_t a_y1, uint16_t a_color, std::uint32_t a_lineThickness,char* a_buffer){

    if(a_x0 > a_x1)
        swapCoordinates(a_x0, a_y0, a_x1, a_y1);

    if(a_y0 == a_y1){
        for(int y=0;y<a_lineThickness;y++){
            std::uint32_t currY = a_y0 + y;
            std::uint64_t len = abs(static_cast<int>(a_x0 - a_x1));
            std::uint64_t xOffset = convertPixelToOffset(a_x0, currY);
            std::uint64_t xOffsetEnd = convertPixelToOffset(a_x0 + len, currY);

            if(xOffset == -1)
                continue;
            std::fill(a_buffer + xOffset,(a_buffer + xOffsetEnd) , a_color);
        }

    }
    else if(a_x0 == a_x1){
        //100, 100, 100, 50,
        if(a_y0 > a_y1)
            swapCoordinates(a_x0, a_y0, a_x1, a_y1);

        for(int y= static_cast<int>(a_y0); y < a_y1; y++){
            //std::uint32_t currY = a_y0 + y;
            std::uint64_t xOffset = convertPixelToOffset(a_x0, y);
            if(xOffset == -1)
                continue;
            std::fill(a_buffer + xOffset, (a_buffer + xOffset) + a_lineThickness, a_color);
        }

    }
}

void Drawing::drawCurve(std::uint32_t a_x0, std::uint32_t a_y0, std::uint32_t a_x2, std::uint32_t a_y2, int a_curveHeight, uint16_t a_color, std::uint32_t a_lineThickness, char* a_buffer) {

    auto pixel = new uint16_t[a_lineThickness];
    std::fill(pixel, pixel + a_lineThickness, a_color);

    int dx = static_cast<int>(a_x2) - static_cast<int>(a_x0);
    int dy = static_cast<int>(a_y2) - static_cast<int>(a_y0);

    double len = sqrt(dx*dx+dy*dy);
    double udx = dx / len;
    double udy = dy / len;

    double px = -udy;
    double py = udx;

    int midX = (static_cast<int>(a_x0) + static_cast<int>(a_x2)) / 2;
    int midY = (static_cast<int>(a_y0) + static_cast<int>(a_y2)) / 2;
    int x1 = static_cast<int>(midX + px * a_curveHeight);
    int y1 = static_cast<int>(midY + py * a_curveHeight);

    int prevY{-1};
    for(int h = 0; h < m_screenHeightPixels; h++)
    {
        double i =  (h - 0.0) / (1080.0 - 0);

        int xa = interpolate( static_cast<int>(a_x0) , x1 , i );
        int ya = interpolate( static_cast<int>(a_y0) , y1 , i );
        int xb = interpolate( x1 , static_cast<int>(a_x2) , i );
        int yb = interpolate( y1 , static_cast<int>(a_y2) , i );

        int x = interpolate( xa , xb , i );
        int y = interpolate( ya , yb , i );

        //ensure each y line on monitor is written to
        if(y == prevY)
            y++;

        std::int64_t xOffset = convertPixelToOffset(x, y);
        if(xOffset == -1){
            continue;
        }
        memcpy(a_buffer + xOffset, pixel, a_lineThickness);
        prevY = y;
    }
    delete[] pixel;
}

void Drawing::drawParkinglinesNew(int a_curveHeight, uint16_t a_color, std::uint32_t a_lineThickness, char* a_buffer) {

    int nrOfLines = 3;

    double percentFromEdgeBottom = 0.05;
    double percentFromEdgeTop = 0.30;
    double percentFromTopBottom = 0.9;
    double percentFromTopTop = 0.20;

    std::uint32_t x0_left = m_screenWidthPixels * percentFromEdgeBottom;
    std::uint32_t x1_left = m_screenWidthPixels * percentFromEdgeTop;
    std::uint32_t y0_left = m_screenHeightPixels * percentFromTopBottom;
    std::uint32_t y1_left = m_screenHeightPixels * percentFromTopTop;

    std::uint32_t x0_right = static_cast<std::uint32_t>(m_screenWidthPixels) * (1.0 - percentFromEdgeBottom);
    std::uint32_t x1_right = m_screenWidthPixels * (1.0 - percentFromEdgeTop);
    std::uint32_t y0_right = m_screenHeightPixels * percentFromTopBottom;
    std::uint32_t y1_right = m_screenHeightPixels * percentFromTopTop;

    ParkingXYK parkingXykLeft{};
    ParkingXYK parkingXykRight{};

    parkingXykLeft = calculateParkingLinesNew(x0_left, y0_left, x1_left - (a_curveHeight / 2), y1_left, a_curveHeight, nrOfLines);
    parkingXykRight = calculateParkingLinesNew(x0_right, y0_right, x1_right - (a_curveHeight / 2), y1_right, a_curveHeight, nrOfLines);

    //assert(parkingXykLeft.x <= (m_screenWidthBytes/2));
    //assert(parkingXykLeft.x <= (m_screenWidthBytes/2));
    //assert(parkingXykLeft.y <= m_screenHeightPixels);
    //assert(parkingXykLeft.y <= m_screenHeightPixels);

    //assert(parkingXykRight.x <= (m_screenWidthBytes/2));
    //assert(parkingXykRight.x <= (m_screenWidthBytes/2));
    //assert(parkingXykRight.y <= m_screenHeightPixels);
    //assert(parkingXykRight.y <= m_screenHeightPixels);

    //red line 0, mid 1 top 2?

    for(int i=0;i<nrOfLines;i++) {
        double leftK = parkingXykLeft.k[i];
        int leftX = parkingXykLeft.x[i] + (a_lineThickness / 2);
        int leftY = parkingXykLeft.y[i];
        int leftM = leftY - (leftK * leftX);

        double rightK = parkingXykRight.k[i];
        int rightX = parkingXykRight.x[i] + (a_lineThickness / 2);
        int rightY = parkingXykRight.y[i];
        int leftYStart = leftY - (a_lineThickness / 2);
        int rightYstart = rightY - (a_lineThickness / 2);
        int rightM = rightY - (rightK * rightX);

        for (int y = 0; y < (a_lineThickness/2); y++) {

            std::uint32_t x0 = (y + leftYStart - leftM) / leftK;
            std::uint32_t x1 = (y + rightYstart - rightM) / rightK;

            if(i == nrOfLines-1)
                drawLine(x0, (y + leftYStart) + 16, x1, (rightY + y), a_color, 2, a_buffer);
            else
                drawLine(x0, (y + leftYStart), x1, (rightY + y), 0xf2e4, 2, a_buffer);
        }
    }





    //mid line
    //drawLine(parkingXykLeft.x[0], parkingXykLeft.y[0], parkingXykRight.x[0] + (a_lineThickness /2), parkingXykRight.y[0], a_color, a_lineThickness, a_buffer);
   // printf("parking: xleft %d yleft %d xright %d yright %d\n", parkingXykLeft.x[0], parkingXykLeft.y[0], parkingXykRight.x[0] + (a_lineThickness /2), parkingXykRight.y[0]);
    drawCurve(x0_left, y0_left, x1_left - (a_curveHeight / 2), y1_right, a_curveHeight, a_color, a_lineThickness, a_buffer);
    drawCurve(x0_right, y0_right, x1_right - (a_curveHeight / 2), y1_right, a_curveHeight, a_color, a_lineThickness, a_buffer);
}

void Drawing::draw4PointShape(std::uint32_t a_x0, std::uint32_t a_y0, std::uint32_t a_x1, std::uint32_t a_y1, std::uint32_t a_x2, std::uint32_t a_y2, std::uint32_t a_x3, std::uint32_t a_y3, uint16_t a_color, std::uint32_t a_lineThickness, char* a_buffer){

    drawLine(a_x0 ,a_y0, a_x1, a_y1, a_color, 2, a_buffer);
    drawLine(a_x1 ,a_y1, a_x2, a_y2, a_color, 2, a_buffer);
    drawLine(a_x2 ,a_y2, a_x3, a_y3, a_color, 2, a_buffer);
    drawLine(a_x0 ,a_y0, a_x3, a_y3, a_color, 2, a_buffer);
}


void Drawing::drawString(unsigned char* a_buffer, std::uint32_t a_color, int a_size, int a_x, int a_y, std::string a_str) {
    int defaultSpacing = 3;

    for(const auto& symbol : a_str){
        drawChar(a_buffer, a_color, a_size, a_x, a_y, symbol);
        a_x += defaultSpacing * a_size * 2;
        if(a_x > m_screenWidthPixels){
            perror("X value outside scope");
        }
    }

}
// Example function to draw a character on the framebuffer
void Drawing::drawChar(unsigned char* a_buffer, std::uint32_t a_color, int a_size, int a_x, int a_y, char a_char) {

    if(a_size < 1 )
        return;

    int XOffset = (m_screenWidthPixels * m_bytesPerPixel) * a_y + (a_x * m_bytesPerPixel);

    std::map<char, std::vector<std::string>> font = {
            {'0', {"01110", "10001", "10011", "10101", "11001", "10001", "01110"}},
            {'1', {"00100", "01100", "00100", "00100", "00100", "00100", "01110"}},
            {'2', {"11110", "00001", "00001", "11110", "10000", "10000", "11111"}},
            {'3', {"11110", "00001", "00001", "11110", "00001", "00001", "11110"}},
            {'4', {"10000", "10000", "10100", "10100", "11111", "00100", "00100"}},
            {'5', {"11111", "10000", "10000", "11110", "00001", "00001", "11110"}},
            {'6', {"01110", "10000", "10000", "11110", "10001", "10001", "01110"}},
            {'7', {"11111", "00001", "00010", "00100", "01000", "01000", "01000"}},
            {'8', {"01110", "10001", "10001", "01110", "10001", "10001", "01110"}},
            {'9', {"01110", "10001", "10001", "01111", "00001", "00001", "01110"}},
            {'0', {"01110", "10001", "10001", "10001", "10001", "10001", "01110"}},
            {'1', {"00100", "01100", "00100", "00100", "00100", "00100", "01110"}},
            {'A', {"01110", "10001", "10001", "11111", "10001", "10001", "10001"}},
            {'B', {"11110", "10001", "10001", "11110", "10001", "10001", "11110"}},
            {'C', {"01110", "10001", "10000", "10000", "10000", "10001", "01110"}},
            {'D', {"11100", "10010", "10001", "10001", "10001", "10010", "11100"}},
            {'E', {"11111", "10000", "10000", "11110", "10000", "10000", "11111"}},
            {'F', {"11111", "10000", "10000", "11110", "10000", "10000", "10000"}},
            {'G', {"01110", "10001", "10000", "10011", "10001", "10001", "01111"}},
            {'H', {"10001", "10001", "10001", "11111", "10001", "10001", "10001"}},
            {'I', {"01110", "00100", "00100", "00100", "00100", "00100", "01110"}},
            {'J', {"00111", "00010", "00010", "00010", "10010", "10010", "01100"}},
            {'K', {"10001", "10010", "10100", "11000", "10100", "10010", "10001"}},
            {'L', {"10000", "10000", "10000", "10000", "10000", "10000", "11111"}},
            {'M', {"10001", "11011", "10101", "10101", "10001", "10001", "10001"}},
            {'N', {"10001", "10001", "11001", "10101", "10011", "10001", "10001"}},
            {'O', {"01110", "10001", "10001", "10001", "10001", "10001", "01110"}},
            {'P', {"11110", "10001", "10001", "11110", "10000", "10000", "10000"}},
            {'Q', {"01110", "10001", "10001", "10001", "10101", "10010", "01101"}},
            {'R', {"11110", "10001", "10001", "11110", "10100", "10010", "10001"}},
            {'S', {"01111", "10000", "10000", "01110", "00001", "00001", "11110"}},
            {'T', {"11111", "00100", "00100", "00100", "00100", "00100", "00100"}},
            {'U', {"10001", "10001", "10001", "10001", "10001", "10001", "01110"}},
            {'V', {"10001", "10001", "10001", "10001", "10001", "01010", "00100"}},
            {'W', {"10001", "10001", "10001", "10101", "10101", "10101", "01010"}},
            {'X', {"10001", "10001", "01010", "00100", "01010", "10001", "10001"}},
            {'Y', {"10001", "10001", "10001", "01010", "00100", "00100", "00100"}},
            {'Z', {"11111", "00001", "00010", "00100", "01000", "10000", "11111"}},
            {'!', {"00100", "00100", "00100", "00100", "00000", "00100", "00100"}},
            {'@', {"01110", "10001", "10011", "10101", "10111", "10000", "01110"}},
            {'#', {"01010", "01010", "11111", "01010", "11111", "01010", "01010"}},
            {'$', {"00100", "01111", "10100", "01110", "00101", "11110", "00100"}},
            {'%', {"11001", "11010", "00010", "00100", "01000", "01011", "10011"}},
            {'^', {"00100", "01010", "10001", "00000", "00000", "00000", "00000"}},
            {'&', {"01100", "10010", "10100", "01000", "10101", "10010", "01101"}},
            {'*', {"00000", "00100", "10101", "01110", "10101", "00100", "00000"}},
            {'(', {"00010", "00100", "01000", "01000", "01000", "00100", "00010"}},
            {')', {"01000", "00100", "00010", "00010", "00010", "00100", "01000"}},
            {'-', {"00000", "00000", "00000", "11111", "00000", "00000", "00000"}},
            {'_', {"00000", "00000", "00000", "00000", "00000", "00000", "11111"}},
            {'=', {"00000", "00000", "11111", "00000", "11111", "00000", "00000"}},
            {'+', {"00000", "00100", "00100", "11111", "00100", "00100", "00000"}},
            {'[', {"01110", "01000", "01000", "01000", "01000", "01000", "01110"}},
            {']', {"01110", "00010", "00010", "00010", "00010", "00010", "01110"}},
            {'{', {"00010", "00100", "01000", "10000", "01000", "00100", "00010"}},
            {'}', {"01000", "00100", "00010", "00001", "00010", "00100", "01000"}},
            {';', {"00000", "00100", "00000", "00100", "00100", "01000", "00000"}},
            {':', {"00000", "00100", "00000", "00000", "00000", "00100", "00000"}},
            {'"', {"01010", "01010", "01010", "00000", "00000", "00000", "00000"}},
            {'\'', {"00100", "00100", "01000", "00000", "00000", "00000", "00000"}},
            {'<', {"00010", "00100", "01000", "10000", "01000", "00100", "00010"}},
            {'>', {"01000", "00100", "00010", "00001", "00010", "00100", "01000"}},
            {'/', {"00001", "00010", "00010", "00100", "01000", "01000", "10000"}},
            {'\\', {"10000", "01000", "01000", "00100", "00010", "00010", "00001"}},
            {'|', {"00100", "00100", "00100", "00100", "00100", "00100", "00100"}},
            {'?', {"01110", "10001", "00001", "00110", "00100", "00000", "00100"}},
            {' ', {"00000", "00000", "00000", "00000", "00000", "00000", "00000"}},
    };


    for(int i=0; i < m_fontHeight * a_size; i++){
        XOffset += m_screenWidthBytes;
        std::string currRowpattern = font.at(a_char)[i/a_size].c_str();

        int j=0;
        for(const auto& pixel : currRowpattern){
            if(pixel == '1'){
                int offset = (j * m_bytesPerPixel*a_size);
                if(offset % 2 != 0)
                    offset++;
                std::fill_n(reinterpret_cast<std::uint16_t*>(a_buffer + XOffset + offset), a_size, a_color);
            }
            j++;
        }
    }
}