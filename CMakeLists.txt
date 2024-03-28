cmake_minimum_required(VERSION 3.27)
project(rpi3test)

set(CMAKE_CXX_STANDARD 20)


include_directories("/opt/poky/3.1.31/sysroots/aarch64-poky-linux/usr/lib/")
#set(TurboJPEG_LIBRARY "/opt/poky/3.1.31/sysroots/aarch64-poky-linux/usr/lib/libturbojpeg.so")

#find_package (JPEG REQUIRED)
#target_add_include_directories(rpi3test ${JPEG_INCLUDE_DIR})
#target_link_libraries(rpi3test ${JPEG_LIBRARIES})

add_executable(cameraApp main.cpp
        webcamStream.h
        graphix/graphix.cpp
        graphix/graphix.h
        jpegConversion/jpegConversion.cpp
        jpegConversion/jpegConversion.h
        circularImageBuffer/circularImageBuffer.h
        circularImageBuffer/circularImageBuffer.cpp
        OBD2/obd2.h
        OBD2/obd2.cpp
)

set_target_properties(cameraApp PROPERTIES INSTALL_RPATH "/opt/poky/3.1.31/sysroots/aarch64-poky-linux/usr/lib")
target_link_libraries(cameraApp -ljpeg -lpthread)