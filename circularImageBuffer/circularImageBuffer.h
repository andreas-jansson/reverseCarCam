#ifndef RPI3TEST_CIRCULARIMAGEBUFFER_H
#define RPI3TEST_CIRCULARIMAGEBUFFER_H

#include <iostream>
#include <mutex>
#include <cstring>
#include <vector>
#include <openssl/sha.h>

void testCircularBuffer();

struct ImageFrame{
    void* frame{};
    std::uint32_t size{};
    std::int64_t frameNr{};
};


class CirularImageBuffer{

    static std::vector<ImageFrame> m_bufferUnProcessed;
    static int m_headUnProcessed;
    static int m_tailUnProcessed;
    static int m_nrOfUnProcessedImages;
    static int m_imageSizeUnProcessed;
    static bool m_isFullUnProcessed;
    static bool m_isEmptyUnProcessed;
    static std::mutex mutexUnProcessed;
    bool m_isAboveThresholdUnProcessed{};
    int m_tresholdLevelUnProcessed{};


    static std::vector<ImageFrame> m_bufferProcessed;
    static int m_headProcessed;
    static int m_tailProcessed;
    static int m_nrOfImagesProcessed;
    static int m_imageSizeProcessed;
    static bool m_isFullProcessed;
    static bool m_isEmptyProcessed;
    static std::mutex mutexProcessed;
    bool m_isAboveThresholdProcessed{};
    int m_tresholdLevelProcessed{};

    static CirularImageBuffer * instance;


    CirularImageBuffer() = default;

public:
    CirularImageBuffer(CirularImageBuffer &other) = delete;

    void operator=(const CirularImageBuffer &) = delete;

    static CirularImageBuffer *GetInstance();

    void initUnprocessedQ(int a_queueSize, int a_size);
    void initProcessedQ(int a_queueSize, int a_size);

    bool enquqeUnprocessedImg(ImageFrame& frame);
    void enquqeProcessedImg(ImageFrame& r_frame);

    int dequeueUnprocessedImg(ImageFrame& r_frame);
    int dequeueProcessedImg(void* r_frame);

    static int getNrofUnproccesedImgs();
    static int getNrofProcessedImgs();

    void *getMJPEGBuffer();
    static int getMJPEGHeadPos();
    static int getMJPEGTailPos();

    void setTresholdStatusUnprocessed(int a_lvl);
    void setTresholdStatusProcessed(int a_lvl);
    bool getTresholdStatusUnprocessed() const;
    bool getTresholdStatusProcessed() const;

    void setTresholdLevelUnprocessed();
    void setTresholdLevelProcessed();

};

#endif //RPI3TEST_CIRCULARIMAGEBUFFER_H
