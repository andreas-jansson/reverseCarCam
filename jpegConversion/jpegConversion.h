#ifndef RPI3TEST_JPEGCONVERSION_H
#define RPI3TEST_JPEGCONVERSION_H

#include <turbojpeg.h>
#include <jpeglib.h>



void decode_jpeg_frame(unsigned char *frame, size_t frame_size,
                       char* output_buffer,
                       int output_width, int output_height, int output_stride);

void decode_jpeg_frame_to_rgb565(unsigned char *frame, size_t frame_size, uint16_t* output_buffer, int output_width, int output_height);

void decode_jpeg_frame_to_rgb565_fast(unsigned char *frame, size_t frame_size, uint16_t* output_buffer, int output_width, int output_height);


void decode_jpeg_frame_to_rgb565_with_context(struct jpeg_decompression_context* ctx, unsigned char *frame, size_t frame_size, uint16_t* output_buffer, int output_width, int output_height);

void cleanup_jpeg_decompression_context(struct jpeg_decompression_context* ctx);

struct jpeg_decompression_context* initialize_jpeg_decompression_context(int output_width);


#endif //RPI3TEST_JPEGCONVERSION_H
