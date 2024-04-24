#include "circularImageBuffer.h"
#include <thread>
#include <vector>

CirularImageBuffer* CirularImageBuffer::instance{};
std::vector<ImageFrame> CirularImageBuffer::m_bufferUnProcessed{};
int CirularImageBuffer::m_headUnProcessed{};
int CirularImageBuffer::m_tailUnProcessed{};
int CirularImageBuffer::m_nrOfUnProcessedImages{};
int CirularImageBuffer::m_imageSizeUnProcessed{};
bool CirularImageBuffer::m_isFullUnProcessed{};
bool CirularImageBuffer::m_isEmptyUnProcessed{true};
std::mutex CirularImageBuffer::mutexUnProcessed;


std::vector<ImageFrame> CirularImageBuffer::m_bufferProcessed{};
int CirularImageBuffer::m_headProcessed{};
int CirularImageBuffer::m_tailProcessed{};
int CirularImageBuffer::m_nrOfImagesProcessed{};
int CirularImageBuffer::m_imageSizeProcessed{};
bool CirularImageBuffer::m_isFullProcessed{};
bool CirularImageBuffer::m_isEmptyProcessed{true};
std::mutex CirularImageBuffer::mutexProcessed;

[[noreturn]] void t_pollCircularBuffer2(int a_bufferSize);
[[noreturn]] void t_dequeueMJPEG(int a_sleepSeconds);
[[noreturn]] void t_enququedata(int a_sleepSeconds);
bool isMJPEGValid(ImageFrame& a_frame);


CirularImageBuffer* CirularImageBuffer::GetInstance(){
    std::lock_guard<std::mutex> lock(mutexUnProcessed);
    if (instance == nullptr)
    {
        instance = new CirularImageBuffer();
    }
    return instance;
}

void CirularImageBuffer::initUnprocessedQ(int a_queueSize, int a_size){
    static bool isInitiated{};
    if(!isInitiated){
        m_imageSizeUnProcessed = a_size;
        m_nrOfUnProcessedImages = a_queueSize;
        m_bufferUnProcessed.resize(a_queueSize);
        for(int i=0; i < m_nrOfUnProcessedImages; i++){
            printf("malloc m_bufferUnProcessed[%d].frame size %d\n", i, a_size);
            m_bufferUnProcessed[i].frame = malloc(a_size);
        }
        isInitiated = true;
        setTresholdLevelUnprocessed();
        printf("initiaged MJPEG buffer: size %d, queueSize %d, bufferSize %d level %d\n", m_imageSizeUnProcessed, m_nrOfUnProcessedImages, a_size * a_queueSize, m_tresholdLevelUnProcessed);

    }
    else{
        printf("already initiated MJPEG buffer: size %d, queueSize %d, bufferSize %d level %d\n", m_imageSizeUnProcessed, m_nrOfUnProcessedImages, a_size * a_queueSize, m_tresholdLevelUnProcessed);
    }
}

void CirularImageBuffer::initProcessedQ(int a_queueSize, int a_size){
    static bool isInitiated{};
    if(!isInitiated){
        m_imageSizeProcessed = a_size;
        m_nrOfImagesProcessed = a_queueSize;
        m_bufferProcessed.resize(a_queueSize);
        for(int i=0; i < m_nrOfImagesProcessed; i++){
            printf("malloc m_bufferProcessed[%d].frame size %d\n", i, a_size);
            m_bufferProcessed[i].frame = malloc(a_size);
        }
        isInitiated = true;
        setTresholdLevelProcessed();
        printf("initiaged RGB565 buffer: size %d, queueSize %d, bufferSize %d level %d\n", m_imageSizeProcessed, m_nrOfImagesProcessed, a_size * a_queueSize, m_tresholdLevelProcessed);
    }
    else{
        printf("initiaged RGB565 buffer: size %d, queueSize %d, bufferSize %d\n", m_imageSizeProcessed, m_nrOfImagesProcessed, a_size * a_queueSize);
    }
}

bool CirularImageBuffer::enquqeUnprocessedImg(ImageFrame& frame){
    std::lock_guard<std::mutex> lock(mutexUnProcessed);
    if(m_isFullUnProcessed){
        //printf("ENQ: head %d tail %d full %d empty %d\n",m_headUnProcessed, m_tailUnProcessed, m_isFullUnProcessed, m_isEmptyUnProcessed);
        m_tailUnProcessed = (m_tailUnProcessed + 1) % m_nrOfUnProcessedImages;
    }

    memcpy(m_bufferUnProcessed[m_headUnProcessed].frame, frame.frame, frame.size);
    m_bufferUnProcessed[m_headUnProcessed].size = frame.size;
    m_bufferUnProcessed[m_headUnProcessed].frameNr = frame.frameNr;

    if(!isMJPEGValid(frame))
        return false;

    m_headUnProcessed = (m_headUnProcessed + 1) % m_nrOfUnProcessedImages;
    m_isFullUnProcessed = (m_headUnProcessed == m_tailUnProcessed);
    m_isEmptyUnProcessed = false;

    int level = m_isFullUnProcessed ? m_nrOfUnProcessedImages : abs(m_tailUnProcessed - m_headUnProcessed);
    setTresholdStatusUnprocessed(level);
    //printf("enq: head %d tail %d full %d empty %d\n",m_headUnProcessed, m_tailUnProcessed, m_isFullUnProcessed, m_isEmptyUnProcessed);
    return true;
}

void CirularImageBuffer::enquqeProcessedImg(ImageFrame& r_frame){
    std::lock_guard<std::mutex> lock(mutexProcessed);
    if(m_isFullProcessed){
        m_tailProcessed = (m_tailProcessed + 1) % m_nrOfImagesProcessed;
    }
    memcpy(m_bufferProcessed[m_headProcessed].frame, r_frame.frame, r_frame.size);
    m_bufferProcessed[m_headProcessed].size = r_frame.size;
    m_bufferProcessed[m_headProcessed].frameNr = r_frame.frameNr;

    m_headProcessed = (m_headProcessed + 1) % m_nrOfImagesProcessed;
    m_isFullProcessed = (m_headProcessed == m_tailProcessed);
    m_isEmptyProcessed = false;

    int level = m_isFullProcessed ? m_nrOfImagesProcessed : abs(m_tailProcessed - m_headProcessed);
    setTresholdStatusProcessed(level);
}


int CirularImageBuffer::dequeueUnprocessedImg(ImageFrame& r_frame) {
    if(m_isEmptyUnProcessed){
        //printf("DEQ: head %d tail %d full %d empty %d\n",m_headUnProcessed, m_tailUnProcessed, m_isFullUnProcessed, m_isEmptyUnProcessed);
        return -1;
    }
    std::lock_guard<std::mutex> lock(mutexUnProcessed);
    if(m_tailUnProcessed == m_headUnProcessed && !m_isFullUnProcessed){
        m_isEmptyUnProcessed = true;
        return -1;
    }

    memcpy(r_frame.frame , m_bufferUnProcessed[m_tailUnProcessed].frame, m_bufferUnProcessed[m_tailUnProcessed].size);
    r_frame.size = m_bufferUnProcessed[m_tailUnProcessed].size;
    r_frame.frameNr = m_bufferUnProcessed[m_tailUnProcessed].frameNr;

    memset(m_bufferUnProcessed[m_tailUnProcessed].frame, 0 , m_bufferUnProcessed[m_tailUnProcessed].size);


    m_tailUnProcessed = (m_tailUnProcessed + 1) % m_nrOfUnProcessedImages;

    m_isFullUnProcessed = false;
    m_isEmptyUnProcessed  = m_tailUnProcessed == m_headUnProcessed;

    int level = abs(m_tailUnProcessed - m_headUnProcessed);
    setTresholdStatusUnprocessed(level);

    //printf("deq: head %d tail %d full %d empty %d\n",m_headUnProcessed, m_tailUnProcessed, m_isFullUnProcessed, m_isEmptyUnProcessed);

    return 1;
}

int CirularImageBuffer::dequeueProcessedImg(void* r_frame) {
    if(m_isEmptyProcessed)
        return -1;
    std::lock_guard<std::mutex> lock(mutexProcessed);
    if(m_tailProcessed == m_headProcessed && !m_isFullProcessed){
        m_isEmptyProcessed = true;
        return -1;
    }

    memcpy(r_frame , m_bufferProcessed[m_tailProcessed].frame, m_bufferProcessed[m_tailProcessed].size);
    //memcpy(r_frame.frame , m_bufferProcessed[m_tailProcessed].frame, m_bufferProcessed[m_tailProcessed].size);
    //r_frame.size = m_bufferProcessed[m_tailProcessed].size;
    //r_frame.frameNr = m_bufferProcessed[m_tailProcessed].frameNr;
    memset(m_bufferProcessed[m_tailProcessed].frame, 0 , m_bufferProcessed[m_tailProcessed].size);

    m_tailProcessed = (m_tailProcessed + 1) % m_nrOfImagesProcessed;
    m_isFullProcessed = false;

    int level = abs(m_tailProcessed - m_headProcessed);
    setTresholdStatusProcessed(level);

    return 1;
}

void CirularImageBuffer::setTresholdStatusUnprocessed(int a_lvl){
    if(a_lvl >= m_tresholdLevelUnProcessed)
        m_isAboveThresholdUnProcessed=true;
    else
        m_isAboveThresholdUnProcessed=false;

}
void CirularImageBuffer::setTresholdStatusProcessed(int a_lvl){
    if(a_lvl >= m_tresholdLevelProcessed)
        m_isAboveThresholdProcessed=true;
    else
        m_isAboveThresholdProcessed=false;
}

void CirularImageBuffer::setTresholdLevelUnprocessed(){
    if(m_nrOfUnProcessedImages == 0)
        perror("m_nrOfUnProcessedImages is not set yet");

    m_tresholdLevelUnProcessed = m_nrOfUnProcessedImages / 3;
    if(m_tresholdLevelUnProcessed == 0)
        perror("m_tresholdLevelUnProcessed is 0!");
}

void CirularImageBuffer::setTresholdLevelProcessed(){
    if(m_nrOfImagesProcessed == 0)
        perror("m_nrOfImagesProcessed is not set yet");

    m_tresholdLevelProcessed = m_nrOfImagesProcessed / 3;
    if(m_tresholdLevelProcessed == 0)
        perror("m_tresholdLevelUnProcessed is 0!");
}

bool CirularImageBuffer::getTresholdStatusUnprocessed() const{
    return m_isAboveThresholdUnProcessed;
}
bool CirularImageBuffer::getTresholdStatusProcessed() const{
    return m_isAboveThresholdProcessed;
}

///////////////////////////// TESTS ////////////////////////////////////////

bool testOverwrite();

bool testEmpty();
/*
void testCircularBuffer(){
    CirularImageBuffer* circleBuf = CirularImageBuffer::GetInstance();
    circleBuf->initUnprocessedQ(10, 1);
    circleBuf->initProcessedQ(10, 1);

    //testOverwrite();
    //testEmpty();
    std::thread t_1(t_enququedata, 1);
    std::thread t_2(t_dequeueMJPEG, 4);
    std::thread t_3(t_pollCircularBuffer2, 10);
    while(true);

    return;

}
 */

/*
bool testOverwrite(){
    CirularImageBuffer* circleBuf = CirularImageBuffer::GetInstance();

    std::string item1 = "hello";
    std::string item2 = "from";
    std::string item3 = "the";
    std::string item4 = "other";
    std::string item5 = "side";
    std::string item6 = "!";
    std::string returnItem{};

    circleBuf->enquqeMJPEGImage(static_cast<void *>(const_cast<char *>(item1.c_str())));
    circleBuf->enquqeMJPEGImage(static_cast<void *>(const_cast<char *>(item2.c_str())));
    circleBuf->enquqeMJPEGImage(static_cast<void *>(const_cast<char *>(item3.c_str())));
    circleBuf->enquqeMJPEGImage(static_cast<void *>(const_cast<char *>(item4.c_str())));
    circleBuf->enquqeMJPEGImage(static_cast<void *>(const_cast<char *>(item5.c_str())));
    circleBuf->enquqeMJPEGImage(static_cast<void *>(const_cast<char *>(item6.c_str())));


    circleBuf->dequeueMJPEGImage(const_cast<char *>(returnItem.c_str()));
    assert(!strcmp(item2.c_str(), returnItem.c_str()));
    returnItem.clear();

    circleBuf->dequeueMJPEGImage(const_cast<char *>(returnItem.c_str()));
    assert(!strcmp(item3.c_str(), returnItem.c_str()));
    returnItem.clear();

    circleBuf->dequeueMJPEGImage(const_cast<char *>(returnItem.c_str()));
    assert(!strcmp(item4.c_str(), returnItem.c_str()));
    returnItem.clear();

    circleBuf->dequeueMJPEGImage(const_cast<char *>(returnItem.c_str()));
    assert(!strcmp(item5.c_str(), returnItem.c_str()));
    returnItem.clear();

    circleBuf->dequeueMJPEGImage(const_cast<char *>(returnItem.c_str()));
    assert(!strcmp(item6.c_str(), returnItem.c_str()));

    printf("completed test\n");
    return true;
}

bool testEmpty(){
    CirularImageBuffer* circleBuf = CirularImageBuffer::GetInstance();

    std::string item1 = "hello";
    std::string item2 = "from";
    std::string item3 = "the";
    std::string item4 = "other";
    std::string item5 = "side";
    std::string item6 = "!";
    std::string returnItem{};

    circleBuf->enquqeMJPEGImage(static_cast<void *>(const_cast<char *>(item1.c_str())));
    circleBuf->enquqeMJPEGImage(static_cast<void *>(const_cast<char *>(item2.c_str())));
    circleBuf->enquqeMJPEGImage(static_cast<void *>(const_cast<char *>(item3.c_str())));
    circleBuf->enquqeMJPEGImage(static_cast<void *>(const_cast<char *>(item4.c_str())));
    circleBuf->enquqeUnprocessedImg(static_cast<void *>(const_cast<char *>(item5.c_str())));
    circleBuf->enquqeMJPEGImage(static_cast<void *>(const_cast<char *>(item6.c_str())));


    circleBuf->dequeueMJPEGImage(const_cast<char *>(returnItem.c_str()));
    assert(!strcmp(item2.c_str(), returnItem.c_str()));
    returnItem.clear();

    circleBuf->dequeueMJPEGImage(const_cast<char *>(returnItem.c_str()));
    assert(!strcmp(item3.c_str(), returnItem.c_str()));
    returnItem.clear();

    circleBuf->dequeueMJPEGImage(const_cast<char *>(returnItem.c_str()));
    assert(!strcmp(item4.c_str(), returnItem.c_str()));
    returnItem.clear();

    circleBuf->dequeueMJPEGImage(const_cast<char *>(returnItem.c_str()));
    assert(!strcmp(item5.c_str(), returnItem.c_str()));
    returnItem.clear();

    circleBuf->dequeueMJPEGImage(const_cast<char *>(returnItem.c_str()));
    assert(!strcmp(item6.c_str(), returnItem.c_str()));

    circleBuf->dequeueUnprocessedImg(const_cast<char *>(returnItem.c_str()));
    assert(returnItem.empty());

    circleBuf->dequeueMJPEGImage(const_cast<char *>(returnItem.c_str()));
    assert(returnItem.empty());

    printf("completed test\n");
    return true;
}

*/

int CirularImageBuffer::getNrofUnproccesedImgs(){
    std::lock_guard<std::mutex> lock(mutexUnProcessed);
    int size = m_isFullUnProcessed ? m_nrOfUnProcessedImages : abs(m_headUnProcessed - m_tailUnProcessed);
    return size;
}


int CirularImageBuffer::getNrofProcessedImgs(){
    std::lock_guard<std::mutex> lock(mutexProcessed);
    int size = m_isFullProcessed ? m_nrOfImagesProcessed : abs(m_headProcessed - m_tailProcessed);
    return size;
}

/*
void* CirularImageBuffer::getMJPEGBuffer(){
    return m_bufferUnProcessed;
}
*/

int CirularImageBuffer::getMJPEGHeadPos(){
    std::lock_guard<std::mutex> lock(mutexUnProcessed);
    return m_headUnProcessed;
}

int CirularImageBuffer::getMJPEGTailPos(){
    std::lock_guard<std::mutex> lock(mutexUnProcessed);
    return m_tailUnProcessed;
}

/*
[[noreturn]] void t_enququedata(int a_sleepSeconds){
    using namespace std::chrono_literals;
    CirularImageBuffer* circleBuf = CirularImageBuffer::GetInstance();
    char x = 'a';
    while(true){
        circleBuf->enquqeUnprocessedImg(reinterpret_cast<void *>(&x));
        std::this_thread::sleep_for(std::chrono::seconds(a_sleepSeconds));
        if(x == 'z')
            x = 'a';
        x++;
    }
}

[[noreturn]] void t_dequeueMJPEG(int a_sleepSeconds){
    using namespace std::chrono_literals;
    CirularImageBuffer* circleBuf = CirularImageBuffer::GetInstance();
    char mjpegImage{};
    char rgb565Image{};

    printf("\nread: ");
    while(true){
        while(circleBuf->dequeueUnprocessedImg(reinterpret_cast<void *>(&mjpegImage)) == -1){
            std::this_thread::sleep_for(10ms);
        }

        x.emplace_back(mjpegImage);

        newData = true;
        //circleBuf->enquqeProcessedImg(reinterpret_cast<void *>(&rgb565Image));
        std::this_thread::sleep_for(std::chrono::seconds(a_sleepSeconds));
    }
}


[[noreturn]] void t_pollCircularBuffer2(int a_bufferSize) {
    using namespace std::chrono_literals;

    CirularImageBuffer* circleBuf = CirularImageBuffer::GetInstance();
    int nrOfMJPEG{};
    int nrOfRGB565{};
    std::vector<char>y;
    y.emplace_back('a');
    y.emplace_back('b');
    y.emplace_back('c');

    while(true){
        char* buf = static_cast<char*>(circleBuf->getMJPEGBuffer());
        int head = circleBuf->getMJPEGHeadPos();
        int tail = circleBuf->getMJPEGTailPos();
        // Overwrite mjpeg status line
        printf("\x1b[A\rjpeg: ");
        for (int i = 0; i < a_bufferSize; i++) {
            if (head == tail)
                printf("(<%c>) ", buf[i]); // Head position
            else if (head == i)
                printf("(%c) ", buf[i]); // Head position
            else if (tail == i)
                printf("<%c> ", buf[i]); // Tail position
            else
                printf("%c ", buf[i]); // Normal buffer element
        }
        printf("\nread: ");
        for (const auto& j : x) {
            printf("%c - ", j);
        }
        fflush(stdout);


        std::this_thread::sleep_for(50ms);
    }
}
*/

bool isMJPEGValid(ImageFrame& a_frame){

    std::uint32_t size = a_frame.size - 1;
    bool isBad{};
    auto image = static_cast<unsigned char*>(a_frame.frame);

    if(size <= 0){
        printf("ERROR BAD SIZE %d\n", size);
        return false;
    }

    if(image[0] != 0xFF || image[1] != 0xD8 )
        isBad = true;
    else if(image[size -1 ] != 0xFF || image[size] != 0xD9 )
        isBad = true;

    if(isBad){
        printf("start: %02x %02x\n",  image[0],  image[1]);
        printf("last:  %02x %02x\n",  image[size-1],  image[size]);

        //std::string name = "bad-" + std::to_string(counter);
        //writeToFile(static_cast<void*>(a_image), static_cast<size_t>(image.bytesused), name);
    }
    return !isBad;
}