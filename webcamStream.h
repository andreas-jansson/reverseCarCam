#ifndef RPI3TEST_WEBCAMSTREAM_H
#define RPI3TEST_WEBCAMSTREAM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <iostream>
#include <chrono>
#include <thread>

#define nrOfBuf 3


int fd;
char* buffer;
struct v4l2_buffer queryBuffer = {0};
enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;



bool init_cam(int a_width, int a_height);

void close_cam();


struct Buffers{
    void* start;
    size_t length;
};

struct Buffers* buffers;

static int xioctl(int fh, int request, void *arg) {
    int r;
    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);
    return r;
}

void close_cam() {
    munmap(buffer, queryBuffer.length);
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
        perror("Stop Capture");
    }
    close(fd);
}

bool init_cam(int a_width, int a_height) {
    const char *device = "/dev/video0";
    fd = open(device, O_RDWR);
    if (fd == -1) {
        perror("Opening video device");
        return false;
    }

    // Query device capabilities
    struct v4l2_capability capability;
    if (ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0) {
        perror("Querying capabilities");
        return false;
    }
    printf("Driver: %s, Card: %s, Version: %u.%u, Capabilities: %08x\n",
           capability.driver, capability.card, (capability.version >> 16) & 0xFF,
           (capability.version >> 8) & 0xFF, capability.capabilities);

    struct v4l2_format imageFormat = {0};
    imageFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    imageFormat.fmt.pix.width = a_width;
    imageFormat.fmt.pix.height = a_height;
    imageFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    imageFormat.fmt.pix.field = V4L2_FIELD_NONE;
    if (ioctl(fd, VIDIOC_S_FMT, &imageFormat) < 0) {
        perror("Setting Pixel Format");
        return false;
    }

    struct v4l2_requestbuffers req = {0};
    req.count = nrOfBuf;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        perror("Requesting Buffer");
        return false;
    }

    buffers = static_cast<Buffers*>(calloc(nrOfBuf, sizeof(*buffers)));
    for (int i = 0; i < nrOfBuf; i++) {
        queryBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        queryBuffer.memory = V4L2_MEMORY_MMAP;
        queryBuffer.index = i;
        if (ioctl(fd, VIDIOC_QUERYBUF, &queryBuffer) < 0) {
            perror("Querying Buffer");
            continue;
        }
        buffers[i].length = queryBuffer.length;
        buffers[i].start = mmap(nullptr, queryBuffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, queryBuffer.m.offset);

        printf("Buffer %d: address=%p, length=%d\n", i, buffers[i].start, buffers[i].length);

        struct v4l2_streamparm streamparm;
        streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (ioctl(fd, VIDIOC_G_PARM, &streamparm) == -1) {
            perror("VIDIOC_G_PARM");
        }

        // Calculate and print the frame rate
        if (streamparm.parm.capture.timeperframe.denominator != 0) {
            double fps = (double)streamparm.parm.capture.timeperframe.denominator /
                         (double)streamparm.parm.capture.timeperframe.numerator;
            printf("Current frame rate: %.2f FPS\n", fps);
        } else {
            printf("Frame rate information not available.\n");
        }

        // Enqueue the buffer
        if (ioctl(fd, VIDIOC_QBUF, &queryBuffer) < 0) {
            perror("Enqueueing buffer");
            continue;
        }
    }

    // Start streaming
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        perror("Starting streaming");
        return false;
    }
    printf("Camera initialized and streaming started.\n");
    return true;
}





#endif //RPI3TEST_WEBCAMSTREAM_H
