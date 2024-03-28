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
#include <netinet/in.h>
#include "webcamStream.h"
#include "graphix/graphix.h"
#include "jpegConversion/jpegConversion.h"
#include "circularImageBuffer/circularImageBuffer.h"
#include "OBD2/obd2.h"


Drawing *Drawing::instance{nullptr};
std::mutex Drawing::mutex_;
std::uint32_t Drawing::m_screenWidthBytes{};
std::uint32_t Drawing::m_screenWidthPixels{};
std::uint32_t Drawing::m_screenHeightPixels{};
std::uint32_t Drawing::m_bytesPerPixel{};
std::uint64_t Drawing::m_screenSizeBytes{};

uint16_t convertYUVtoRGB565(int Y, int U, int V);

void YUV422toRGB565(const uint8_t *yuv, uint16_t *rgb565, std::uint32_t pixelCount);

char *init_screen(std::uint32_t &width, std::uint32_t &height, std::uint32_t &screensize);

void display_splashscreen();

void drawDots(char* a_buffer, int a_x, int a_y, int a_width, int a_height);

void print_avg_time_us(char *a_msg, int a_totalSamples, int a_currentIteration, std::int64_t &a_usTime);

bool writeToFile(const void *data, size_t size, const std::string &filePath);

void mirror_image(std::uint16_t* a_buffer, int width, int height);

[[noreturn]] void t_mjpegToRGB565(void *arg);

[[noreturn]] void t_enquqeImages(void *arg);

[[noreturn]] void t_pollCircularBuffer(void *arg);

// Assuming RGB565 pixel structure
struct RGB565Pixel {
    uint16_t value;
};
void resizeRGB565(const RGB565Pixel* srcBuffer, RGB565Pixel* destBuffer, int srcWidth, int srcHeight, int destWidth, int destHeight);

void startServer(void* arg);

int min(int a, int b) {
    return (a < b) ? a : b;
}


int main2() {

    //testCircularBuffer();

    return 0;
}

int main() {
    printf("started\n");
    using namespace std::chrono_literals;

    std::uint32_t width{}, height{}, screensize{};
    char *fbp;

    if ((fbp = init_screen(width, height, screensize)) == nullptr)
        return 1;

    printf("screen init\n");
    init_cam(1920, 1080);
    printf("cam init\n");

    Drawing *drawing = Drawing::GetInstance();
    drawing->init(width, height, 2);

    //CirularImageBuffer* circleBuf = CirularImageBuffer::GetInstance();
    //circleBuf->initUnprocessedQ(5, 1920*1080*2);
    //circleBuf->initProcessedQ(5, screensize);
    //std::thread t1_imageFetcher(t_enquqeImages, nullptr);
    //std::thread t1_imageProcessor(t_mjpegToRGB565, nullptr);
    //std::thread t2_imageProcessor(t_mjpegToRGB565, nullptr);
    //std::thread t3_imageProcessor(t_mjpegToRGB565, nullptr);
    //std::thread t4_imageProcessor(t_mjpegToRGB565, nullptr);
    //std::thread t1_imageStats(t_pollCircularBuffer, nullptr);

    display_splashscreen();

    char *displayBuf = new char[1920 * 1080 * 2];

    int curve{};
    bool isLeft{};

    //fps counter
    std::uint64_t usTime{};
    std::uint64_t counter{}, fpsCounter{};
    auto prevSec = std::chrono::high_resolution_clock::now();
    auto prevFrame = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();



    const char* address = "192.168.0.10"; // IP address of the Vgate iCar V2 device
    int port = 35000; // Port number used by the Vgate iCar V2 device
    std::string command = "010C\r"; // Command to request vehicle speed in OBD-II
    std::thread t1_obd2Sender(sendUdpMessage, address, port, command);
    std::thread t1_obd2Listner(listenForUdpMessages, nullptr);
    std::thread t1_startServer(startServer, nullptr);



    while (true) {

        //while (circleBuf->dequeueProcessedImg(displayBuf) == -1){
        //    //printf("waiting for rgb565...\n");
        //    std::this_thread::sleep_for(5ms);
        //}

        if (xioctl(fd, VIDIOC_DQBUF, &queryBuffer) < 0) {
            perror("Dequeue Buffer");
            break;
        }


        YUV422toRGB565(static_cast<uint8_t *>(buffers[0].start), reinterpret_cast<uint16_t *>(displayBuf),
                       width * height);

        mirror_image(reinterpret_cast<uint16_t *>(displayBuf),width, height);

        if (xioctl(fd, VIDIOC_QBUF, &queryBuffer) < 0) {
            perror("Queue Buffer");
            break;
        }

        drawing->drawParkinglinesNew(curve, 0xee1f/*0xeea2*/, 20, displayBuf);
        drawing->drawString(reinterpret_cast<unsigned char *>(displayBuf), 0x8fd9, 8,width * 0.05, height * 0.9, "GEAR:");
        drawing->drawString(reinterpret_cast<unsigned char *>(displayBuf), 0xf9a0, 8,width * 0.05, height * 0.9, "     R");
        drawing->drawString(reinterpret_cast<unsigned char *>(displayBuf), 0x8fd9, 8,width * 0.05, height * 0.05, "OBD2");

        memcpy(fbp, displayBuf, screensize);

        //Fps calculation
        now = std::chrono::high_resolution_clock::now();

        usTime += std::chrono::duration_cast<std::chrono::microseconds>(now-prevFrame).count();

        prevFrame = now;

        counter++;
        fpsCounter++;

        //if(counter % 30 == 0){
        //    printf("frame time: %lu us - %lu ms\n", usTime / 30, (usTime/1000)/30);
        //    usTime = 0;
        //}

        //if(std::chrono::duration_cast<std::chrono::seconds>(now-prevSec).count() >= 1){
        //    prevSec = now;
        //    printf("fps: %lu\n", fpsCounter);
        //    fpsCounter = 0;
        //}

        //simulating rotating steering wheel position for testing lines
        if (curve <= 200 && !isLeft) {
            curve += 10;
            if (curve >= 200)
                isLeft = true;
        } else {
            curve -= 10;
            if (curve <= -200)
                isLeft = false;
        }

       // if (xioctl(fd, VIDIOC_QBUF, &queryBuffer) < 0) {
       //     perror("Queue Buffer");
       //     break;
       // }
    }

    display_splashscreen();
    // Cleanup
    munmap(fbp, screensize);
    free(displayBuf);
    close_cam();
    return 0;
}

char* init_screen(std::uint32_t &width, std::uint32_t &height, std::uint32_t &screensize) {


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

    //printf("red len %d\n",vinfo.red.length);
    //printf("green len %d\n",vinfo.green.length);
    //printf("blue len %d\n",vinfo.blue.length);

    printf("virt res: %d x %d\n", vinfo.xres_virtual, vinfo.yres_virtual);
    printf("real res: %d x %d\n", vinfo.xres, vinfo.yres);

    width = vinfo.xres_virtual;
    height = vinfo.yres_virtual;

    // Size of the framebuffer
    screensize = vinfo.yres_virtual * vinfo.xres_virtual * vinfo.bits_per_pixel / 8; // 16 bits depth / bytes = number of bytes
    printf("screensize: %d = %d * %d * %d / 8\n", screensize, vinfo.yres_virtual, vinfo.xres_virtual,
           vinfo.bits_per_pixel);

    // Map framebuffer to user memory
    char *fbp = (char *) mmap(nullptr, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fbp == (void *) -1) {
        perror("Failed to mmap");
        return nullptr;
    }

    if(system("setterm -cursor off") == -1)
        perror("Failed to disable cursor");

    return fbp;
}

// Function to convert a single YUV pixel to RGB565
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

// Main conversion function
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

std::vector<unsigned char> readEDID(const std::string &path) {
    std::ifstream edidFile(path, std::ios::binary);
    std::vector<unsigned char> edidData;

    if (!edidFile) {
        std::cerr << "Cannot open EDID file: " << path << std::endl;
        return edidData; // Return empty if failed to open
    }

    // Read the binary data into the vector
    std::copy(
            std::istream_iterator<unsigned char>(edidFile),
            std::istream_iterator<unsigned char>(),
            std::back_inserter(edidData));

    return edidData;
}

[[noreturn]] void t_mjpegToRGB565([[maybe_unused]]void *arg) {
    using namespace std::chrono_literals;
    struct jpeg_decompression_context *ctx = initialize_jpeg_decompression_context(1920);
    CirularImageBuffer *circleBuf = CirularImageBuffer::GetInstance();
    ImageFrame mjpegImage{};
    mjpegImage.frame = new char[1080 * 1920 * 2];
    ImageFrame rgb565Image;
    rgb565Image.frame = new char[1080 * 1920 * 2];
    rgb565Image.size = 1080 * 1920 * 2;
    int cpu = sched_getcpu();
    printf("t_mjpegToRGB565 spawned! Core %d\n", cpu);

    while (true) {
        while (!circleBuf->getTresholdStatusUnprocessed()) {
            std::this_thread::sleep_for(5ms);
        }

        while (circleBuf->dequeueUnprocessedImg(mjpegImage) == -1) {
            std::this_thread::sleep_for(5ms);
        }

        decode_jpeg_frame_to_rgb565_with_context(ctx,
                                                 static_cast<unsigned char *>(mjpegImage.frame),
                                                 mjpegImage.size,
                                                 reinterpret_cast<uint16_t *>(rgb565Image.frame),
                                                 1920, 1080);

        circleBuf->enquqeProcessedImg(rgb565Image);
        std::this_thread::sleep_for(1us);
    }
}

[[noreturn]] void t_enquqeImages([[maybe_unused]]void *arg) {
    using namespace std::chrono_literals;
    CirularImageBuffer *circleBuf = CirularImageBuffer::GetInstance();
    int cpu = sched_getcpu();
    std::int64_t frameNumber{};
    printf("t_enquqeImages spawned! core %d\n", cpu);

    while (true) {

        if (xioctl(fd, VIDIOC_DQBUF, &queryBuffer) < 0) {
            perror("Dequeue Buffer");
        }
        ImageFrame frame;
        frame.frame = buffers[0].start;
        frame.size = queryBuffer.bytesused;
        frame.frameNr = queryBuffer.bytesused;


        if (!circleBuf->enquqeUnprocessedImg(frame)) {
            perror("Failed enquqeUnprocessedImg");

            if (xioctl(fd, VIDIOC_QBUF, &queryBuffer) < 0) {
                perror("Queue Buffer");
            }
            continue;
        }

        frameNumber++;
        std::this_thread::sleep_for(16ms);

        if (xioctl(fd, VIDIOC_QBUF, &queryBuffer) < 0) {
            perror("Queue Buffer");
        }

    }

}

[[noreturn]] void t_pollCircularBuffer([[maybe_unused]]void *arg) {
    using namespace std::chrono_literals;

    CirularImageBuffer *circleBuf = CirularImageBuffer::GetInstance();
    int nrOfMJPEG;
    int nrOfRGB565;

    while (true) {
        nrOfMJPEG = circleBuf->getNrofUnproccesedImgs();
        nrOfRGB565 = circleBuf->getNrofProcessedImgs();
        printf("\rmjpeg: %-2d rgb: %-2d", nrOfMJPEG, nrOfRGB565);

        flush(std::cout);
        std::this_thread::sleep_for(100ms);
    }
}


void print_avg_time_us(char *a_msg, int a_totalSamples, int a_currentIteration, std::int64_t &a_usTime) {

    if (a_currentIteration % a_totalSamples == 0 && a_currentIteration != 0) {
        printf("avg %s took %ldus\n", a_msg, a_usTime / a_totalSamples);
        a_usTime = 0;
    }
}

bool writeToFile(const void *data, size_t size, const std::string &filePath) {

    std::ofstream file(filePath, std::ios::binary);

    if (!file) {
        std::cerr << "Error: Unable to open file " << filePath << " for writing.\n";
        return false;
    }

    file.write(static_cast<const char *>(data), size);

    if (!file) {
        std::cerr << "Error: Failed to write data to file " << filePath << ".\n";
        return false;
    }

    file.close();
    return true;
}

void display_splashscreen() {

// Open the JPEG file
    struct jpeg_decompress_struct cinfo{};
    struct jpeg_error_mgr jerr{};
    FILE *infile;
    const char *filename = "/home/root/carsplash1080p.jpg";
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
    printf("screen width %d height %d\n", vinfo.xres_virtual,  vinfo.yres_virtual);

    // Map framebuffer to user memory
    char *fbp = (char *) mmap(nullptr, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fbp == (void *) -1) {
        perror("Failed to mmap");
        return;
    }

    // Buffer for the raw JPEG data
    unsigned long data_size = cinfo.output_width * cinfo.output_height * cinfo.output_components;
    printf("jpg width %d height %d\n", cinfo.output_width, cinfo.output_height);
    auto *raw_image = static_cast<unsigned char*>(malloc(data_size));

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
    for (int y = 0; y < min(static_cast<int>(cinfo.output_height), static_cast<int>(vinfo.yres_virtual)); y++) {
        for (int x = 0; x < min(static_cast<int>(cinfo.output_width), static_cast<int>(vinfo.xres_virtual)); x++) {
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

    // Cleanup
    free(raw_image);
    munmap(fbp, screensize);
    using namespace std::chrono_literals;
    close(fb_fd);
    std::this_thread::sleep_for(3s);
}

//a_size is number of pixels
void mirror_image(std::uint16_t* a_buffer, int width, int height) {
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width / 2; x++) {
            int index1 = y * width + x;
            int index2 = y * width + (width - 1 - x);

            // Swap the pixel at index1 and index2
            std::uint16_t temp = a_buffer[index1];
            a_buffer[index1] = a_buffer[index2];
            a_buffer[index2] = temp;
        }
    }
}



void startServer(void *arg) {
    int port = 8080;
    int server_fd, new_socket;
    struct sockaddr_in address{}, cliaddr{};
    int opt = 1;
    int addrlen = sizeof(address);
    unsigned int len = sizeof(cliaddr);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Forcefully attaching socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    int n = recvfrom(new_socket, (char *)buffer, 1024, MSG_WAITALL, (struct sockaddr *) &cliaddr, &len);
    std::cout<<"recieved "<<n<<" bytes\n";
    // Use new_socket to receive data and handle/display it using SDL
}