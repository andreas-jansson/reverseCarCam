#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <linux/fb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fstream>
#include <iterator>
#include <thread>
#include <iostream>
#include <map>
#include <vector>
#include <jpeglib.h>

#include "webcamStream.h"
#include "graphix/graphix.h"
#include "socketListner/socketListner.h"
#include "canDataHandler/canDataHandler.h"


uint16_t convertYUVtoRGB565(int Y, int U, int V);
void YUV422toRGB565(const uint8_t *yuv, uint16_t *rgb565, std::uint32_t pixelCount);
char *init_screen(std::uint32_t &width, std::uint32_t &height, std::uint32_t &screensize);
void display_splashscreen(std::string a_filePath);
void print_avg_time_us(char *a_msg, int a_totalSamples, int a_currentIteration, std::int64_t &a_usTime);
void mirror_image(std::uint16_t *a_buffer, int width, int height);
bool is_connected_to_wifi();
std::int32_t wheel_pos_to_curve_height(std::int32_t a_wheelPos);
double smooth_radian_progression(std::int32_t a_wheelPos);
void disable_sound();


int main() {

    using namespace std::chrono_literals;
    std::uint32_t width{}, height{}, screensize{};
    std::string splashFilePath = "/home/andreas/carsplash1080p.jpg";
    char *fbp;
    bool isWifiActive{false};
    bool cameraActive{true};
    int rpmThreshold = 2000;
    int secondsToWait = 10;


    if ((fbp = init_screen(width, height, screensize)) == nullptr)
        return 1;

    if (!init_cam(1920, 1080))
        return 1;

    CanDataHandler *dataHandler = CanDataHandler::GetInstance();
    dataHandler->configure(10000,0,255,40,progressBarFormat, 790
    );

    Drawing *drawing = Drawing::GetInstance();
    drawing->init(width, height, 2);

    display_splashscreen(splashFilePath);
    char *displayBuf = new char[width * height * 2];

    auto prevSec = std::chrono::high_resolution_clock::now();
    auto prevFrame = std::chrono::high_resolution_clock::now();
    auto prevWifiCheck = std::chrono::high_resolution_clock::now();
    auto timeSinceLowRpm = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();

    std::thread t1_socketServer(t_socketServer, nullptr);

    while (true) {

        now = std::chrono::high_resolution_clock::now();
        if(std::chrono::duration_cast<std::chrono::seconds>(prevWifiCheck - now).count() > 5){
            prevWifiCheck = std::chrono::high_resolution_clock::now();
            isWifiActive = is_connected_to_wifi();
        }

        //determine if the camera system should continue or go to standby
        if(dataHandler->get_rpm() < rpmThreshold){
            timeSinceLowRpm = std::chrono::high_resolution_clock::now();
            if(!cameraActive){
                if (!init_cam(1920, 1080))
                    exit(1);
                else
                    cameraActive = true;
            }
        }
        else if(dataHandler->get_rpm() > rpmThreshold && std::chrono::duration_cast<std::chrono::seconds>(timeSinceLowRpm - now).count() > secondsToWait){
            if(cameraActive){
                cameraActive = false;
                close_cam();
                std::this_thread::sleep_for(5s);
            }
            else{
                std::this_thread::sleep_for(5s);
            }
            continue;
        }


        for (int i = 0; i < nrOfBuf; i++) {
            if (xioctl(fd, VIDIOC_DQBUF, &queryBuffer) < 0) {
                perror("Dequeue Buffer");
                exit(1);
            }
            if (queryBuffer.bytesused <= 0) {
                printf("empty buffer %d\n", i);
                continue;
            }
            YUV422toRGB565(static_cast<uint8_t *>(buffers[queryBuffer.index].start),
                           reinterpret_cast<uint16_t *>(displayBuf),
                           width * height);

            if (xioctl(fd, VIDIOC_QBUF, &queryBuffer) < 0) {
                perror("Queue Buffer");
                exit(1);
            }

            mirror_image(reinterpret_cast<uint16_t *>(displayBuf), width, height);

            int curveHeight = wheel_pos_to_curve_height(dataHandler->get_wheel_position());
            drawing->drawParkinglinesNew(curveHeight, smooth_radian_progression(dataHandler->get_wheel_position()), 0xee1f, 30, displayBuf);

            drawing->drawString(reinterpret_cast<unsigned char*>(displayBuf), 0x8fd9, 8, width * 0.05, height * 0.9,
                                "GEAR:");
            drawing->drawString(reinterpret_cast<unsigned char*>(displayBuf), 0xf9a0, 8, width * 0.05, height * 0.9,
                                "     R");
            drawing->drawString(reinterpret_cast<unsigned char*>(displayBuf), 0xf9a0, 8, width * 0.8, height * 0.9,
                                std::to_string(dataHandler->get_rpm()));
            if(isWifiActive)
                drawing->drawString(reinterpret_cast<unsigned char *>(displayBuf), 0x8fd9, 8, width * 0.05, height * 0.05,
                                "WIFI");
            else
                drawing->drawString(reinterpret_cast<unsigned char *>(displayBuf), 0xf9a0, 8, width * 0.05, height * 0.05,
                                    "!WIFI");


            memcpy(fbp, displayBuf, screensize);
            std::this_thread::sleep_for(5ms);
        }
    }
    display_splashscreen(splashFilePath);
    munmap(fbp, screensize);
    free(displayBuf);
    close_cam();
    return 0;
}

char *init_screen(std::uint32_t &width, std::uint32_t &height, std::uint32_t &screensize) {


    int fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd == -1) {
        perror("Error opening framebuffer device");
        return nullptr;
    }
    struct fb_var_screeninfo vinfo{};
    vinfo.bits_per_pixel = 16;
    vinfo.xres_virtual = 1920;
    vinfo.yres_virtual = 1080;
    vinfo.xres = 1920;
    vinfo.yres = 1080;


    if (ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vinfo)) {
        perror("Error setting variable information");
        //close(fb_fd);
        //exit(3);
    }

    // Get variable screen information
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information");
        return nullptr;
    }

    printf("virt res: %d x %d\n", vinfo.xres_virtual, vinfo.yres_virtual);
    printf("real res: %d x %d\n", vinfo.xres, vinfo.yres);

    width = vinfo.xres_virtual;
    height = vinfo.yres_virtual;

    // Size of the framebuffer
    screensize = vinfo.yres_virtual * vinfo.xres_virtual * vinfo.bits_per_pixel /
                 8; // 16 bits depth / bytes = number of bytes
    printf("screensize: %d = %d * %d * %d / 8\n", screensize, vinfo.yres_virtual, vinfo.xres_virtual,
           vinfo.bits_per_pixel);

    // Map framebuffer to user memory
    char *fbp = (char *) mmap(nullptr, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fbp == (void *) -1) {
        perror("Failed to mmap");
        return nullptr;
    }

    if (system("setterm -cursor off") == -1)
        perror("Failed to disable cursor");

    return fbp;
}

uint16_t convertYUVtoRGB565(int Y, int U, int V) {
    int C = Y - 16;
    int D = U - 128;
    int E = V - 128;

    int R = (298 * C + 409 * E + 128) >> 8;
    int G = (298 * C - 100 * D - 208 * E + 128) >> 8;
    int B = (298 * C + 516 * D + 128) >> 8;

    // Clipping RGB values to be within [0, 255]
    R = R < 0 ? 0 : R > 255 ? 255 : R;
    G = G < 0 ? 0 : G > 255 ? 255 : G;
    B = B < 0 ? 0 : B > 255 ? 255 : B;

    // Converting to RGB565
    uint16_t RGB565 = ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3);
    return RGB565;
}

void YUV422toRGB565(const uint8_t *yuv, uint16_t *rgb565, std::uint32_t pixelCount) {
    for (int i = 0; i < pixelCount; i += 2) {
        int Y1 = yuv[i * 2];
        int U = yuv[i * 2 + 1];
        int Y2 = yuv[i * 2 + 2];
        int V = yuv[i * 2 + 3];

        rgb565[i] = convertYUVtoRGB565(Y1, U, V);
        rgb565[i + 1] = convertYUVtoRGB565(Y2, U, V);
    }
}

void print_avg_time_us(char *a_msg, int a_totalSamples, int a_currentIteration, std::int64_t &a_usTime) {

    if (a_currentIteration % a_totalSamples == 0 && a_currentIteration != 0) {
        printf("avg %s took %ldus\n", a_msg, a_usTime / a_totalSamples);
        a_usTime = 0;
    }
}

void display_splashscreen(std::string a_filePath) {
    // Open the JPEG file
    struct jpeg_decompress_struct cinfo{};
    struct jpeg_error_mgr jerr{};
    FILE *infile;
    const char *filename = a_filePath.c_str();
    if ((infile = fopen(filename, "rb")) == nullptr) {
        fprintf(stderr, "can't open %s\n", filename);
        return;
    }

    // Set up the error handler
    cinfo.err = jpeg_std_error(&jerr);
    // Initialize the JPEG decompression object
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    // Read the header
    jpeg_read_header(&cinfo, TRUE);
    // Start decompression
    jpeg_start_decompress(&cinfo);

    // Open the framebuffer device file
    int fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd == -1) {
        perror("Error opening framebuffer device");
        return;
    }

    struct fb_var_screeninfo vinfo{};
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information");
        return;
    }

    // Size of the framebuffer
    int screensize = static_cast<int>(vinfo.yres_virtual * vinfo.xres_virtual * vinfo.bits_per_pixel / 8);
    printf("screen width %d height %d\n", vinfo.xres_virtual, vinfo.yres_virtual);

    // Map framebuffer to user memory
    char *fbp = (char *) mmap(nullptr, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fbp == (void *) -1) {
        perror("Failed to mmap");
        return;
    }

    // Buffer for the raw JPEG data
    unsigned long data_size = cinfo.output_width * cinfo.output_height * cinfo.output_components;
    printf("jpg width %d height %d\n", cinfo.output_width, cinfo.output_height);
    auto *raw_image = static_cast<unsigned char *>(malloc(data_size));

    // Read the data
    while (cinfo.output_scanline < cinfo.output_height) {
        unsigned char *buffer_array[1];
        buffer_array[0] = raw_image + (cinfo.output_scanline) * cinfo.output_width * cinfo.output_components;
        jpeg_read_scanlines(&cinfo, buffer_array, 1);
    }

    // Finish decompression
    jpeg_finish_decompress(&cinfo);

    // Clean up
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    // Assuming you've already got the correct color offsets and lengths
    int red_offset = static_cast<int>(vinfo.red.offset);
    int green_offset = static_cast<int>(vinfo.green.offset);
    int blue_offset = static_cast<int>(vinfo.blue.offset);

    int red_length = static_cast<int>(vinfo.red.length);
    int green_length = static_cast<int>(vinfo.green.length);
    int blue_length = static_cast<int>(vinfo.blue.length);

// Assuming JPEG is RGB24
    for (int y = 0; y < std::min(static_cast<int>(cinfo.output_height), static_cast<int>(vinfo.yres_virtual)); y++) {
        for (int x = 0; x < std::min(static_cast<int>(cinfo.output_width), static_cast<int>(vinfo.xres_virtual)); x++) {
            // Calculate the position in the raw image buffer
            int color_offset = static_cast<int>((x + y * cinfo.output_width) * cinfo.output_components);

            // Extract the RGB components
            unsigned char r = raw_image[color_offset];
            unsigned char g = raw_image[color_offset + 1];
            unsigned char b = raw_image[color_offset + 2];

            // Calculate the position in the framebuffer's buffer
            int fb_offset = static_cast<int>((x + y * vinfo.xres_virtual) * (vinfo.bits_per_pixel / 8));

            // Prepare the pixel value according to the framebuffer's bit layout
            unsigned int pixel_value = ((r >> (8 - red_length)) << red_offset) |
                                       ((g >> (8 - green_length)) << green_offset) |
                                       ((b >> (8 - blue_length)) << blue_offset);

            // Write the pixel to the framebuffer
            *((unsigned int *) (fbp + fb_offset)) = pixel_value;
        }
    }

    free(raw_image);
    munmap(fbp, screensize);
    using namespace std::chrono_literals;
    close(fb_fd);
    std::this_thread::sleep_for(3s);
}

void mirror_image(std::uint16_t *a_buffer, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width / 2; x++) {
            int index1 = y * width + x;
            int index2 = y * width + (width - 1 - x);

            // Swap the pixel at index1 and index2
            std::uint16_t temp = a_buffer[index1];
            a_buffer[index1] = a_buffer[index2];
            a_buffer[index2] = temp;
        }
    }
}

bool is_connected_to_wifi() {
    if (!system("ping -c 1 192.168.1.2 /dev/null 2>&1"))
        return true;
    else
        return false;
}

std::int32_t wheel_pos_to_curve_height(std::int32_t a_wheelPos) {
    return a_wheelPos*10;
}

double smooth_radian_progression(std::int32_t a_wheelPos) {
    static double currentWheelPos{-6666.0};  // Initialized only once

    double targetRadian = std::abs(a_wheelPos) * (M_PI / 180);  // Convert degrees to radians, use M_PI for pi
    double stepSize = abs(currentWheelPos - targetRadian) > 3? 1.0f : 0.02f;

    // Check if it's the first call
    if (currentWheelPos == -6666.0) {
        currentWheelPos = targetRadian;
        return targetRadian;
    }

    // Check if the target position is already reached
    if (targetRadian == currentWheelPos)
        return targetRadian;


    // Determine the direction of the step, and make sure it does not overshoot the target
    if (currentWheelPos < targetRadian)
        currentWheelPos += std::min(stepSize, targetRadian - currentWheelPos);
    else
        currentWheelPos -= std::min(stepSize, currentWheelPos - targetRadian);

    return currentWheelPos;
}

