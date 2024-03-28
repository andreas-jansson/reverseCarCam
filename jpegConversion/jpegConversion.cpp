#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <turbojpeg.h>
#include <jpeglib.h>
#include <csetjmp>
#include <chrono>
#include <jerror.h>
#include <cstdint>

#define NEW true
#ifdef NEW
////////////////////////////// from github //////////////////////////////////////

#define SIZEOF(object)    ((size_t) sizeof(object))

/* Expanded data source object for mem input */

typedef struct {
    struct jpeg_source_mgr pub;    /* public fields */

    unsigned char *inbuf;         /* Source buffer */
    int nbytes;
    JOCTET *buffer;        /* start of buffer */
    boolean start_of_file;    /* have we gotten any data yet? */
} my_source_mgr;

typedef my_source_mgr *my_src_ptr;

#define INPUT_BUF_SIZE  1024    /* choose an efficiently fread'able size */


/*
 * Initialize source --- called by jpeg_read_header
 * before any data is actually read.
 */

METHODDEF(void)

init_source(j_decompress_ptr cinfo) {
    my_src_ptr src = (my_src_ptr) cinfo->src;
    src->start_of_file = TRUE;
}



METHODDEF(boolean)
fill_input_buffer(j_decompress_ptr cinfo) {
    int i = 0;
    my_src_ptr src = (my_src_ptr) cinfo->src;
    size_t nbytes;

    src->buffer = (JOCTET *) src->inbuf;

    nbytes = src->nbytes;

    if (nbytes <= 0) {
        if (src->start_of_file)    /* Treat empty input file as fatal error */
            ERREXIT(cinfo, JERR_INPUT_EMPTY);
        WARNMS(cinfo, JWRN_JPEG_EOF);


/* Insert a fake EOI marker */
        src->buffer[0] = (JOCTET) 0xFF;
        src->buffer[1] = (JOCTET) JPEG_EOI;
        nbytes = 2;
    }

    i = 0;
    if (src->start_of_file) {
        for (i = 0; i < nbytes && !src->buffer[i]; i++) continue;
    }

    src->pub.next_input_byte = src->buffer + i;
    src->pub.bytes_in_buffer = nbytes - i;
    src->start_of_file = FALSE;

    return TRUE;
}


/*
 * Skip data --- used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 *
 * Writers of suspendable-input applications must note that skip_input_data
 * is not granted the right to give a suspension return.  If the skip extends
 * beyond the data currently in the buffer, the buffer can be marked empty so
 * that the next read will cause a fill_input_buffer call that can suspend.
 * Arranging for additional bytes to be discarded before reloading the input
 * buffer is the application writer's problem.
 */

METHODDEF(void)
skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
    my_src_ptr src = (my_src_ptr) cinfo->src;

/* Just a dumb implementation for now.  Could use fseek() except
 * it doesn't work on pipes.  Not clear that being smart is worth
 * any trouble anyway --- large skips are infrequent.
 */
    if (num_bytes > 0) {
        while (num_bytes > (long) src->pub.bytes_in_buffer) {
            num_bytes -= (long) src->pub.bytes_in_buffer;
            (void) fill_input_buffer(cinfo);
/* note we assume that fill_input_buffer will never return FALSE,
 * so suspension need not be handled.
 */
        }
        src->pub.next_input_byte += (size_t) num_bytes;
        src->pub.bytes_in_buffer -= (size_t) num_bytes;
    }
}


/*
 * An additional method that can be provided by data source modules is the
 * resync_to_restart method for error recovery in the presence of RST markers.
 * For the moment, this source module just uses the default resync method
 * provided by the JPEG library.  That method assumes that no backtracking
 * is possible.
 */


/*
 * Terminate source --- called by jpeg_finish_decompress
 * after all data has been read.  Often a no-op.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */

METHODDEF(void)
term_source(j_decompress_ptr cinfo) {
/* no work necessary here */
}


/*
 * Prepare for input from a filedes or socket.
 * The caller must have already opened the mem, and is responsible
 * for closing it after finishing decompression.
 */

GLOBAL(void)
jpeg_memory_src(j_decompress_ptr cinfo, unsigned char *data, int size) {
    my_src_ptr src;

/* The source object and input buffer are made permanent so that a series
 * of JPEG images can be read from the same file by calling jpeg_mem_src
 * only before the first one.  (If we discarded the buffer at the end of
 * one image, we'd likely lose the start of the next one.)
 * This makes it unsafe to use this manager and a different source
 * manager serially with the same JPEG object.  Caveat programmer.
 */
    if (cinfo->src == NULL) {    /* first time for this JPEG object? */
        cinfo->src = (struct jpeg_source_mgr *)
                (*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                           SIZEOF(my_source_mgr));
        src = (my_src_ptr) cinfo->src;
        src->buffer = (JOCTET *)
                (*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                           INPUT_BUF_SIZE * SIZEOF(JOCTET));
    }

    src = (my_src_ptr) cinfo->src;
    src->pub.init_source = init_source;
    src->pub.fill_input_buffer = fill_input_buffer;
    src->pub.skip_input_data = skip_input_data;
    src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
    src->pub.term_source = term_source;
    src->inbuf = data;
    src->nbytes = size;
    src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
    src->pub.next_input_byte = NULL; /* until buffer loaded */
}


#else

///////////////// jpeg_mem_src replacment from stackoverflow as it's missing in this version /////////////

/* Read JPEG image from a memory segment */
void init_source (j_decompress_ptr cinfo) {}
boolean fill_input_buffer (j_decompress_ptr cinfo)
{

    ERREXIT(cinfo, JERR_INPUT_EMPTY);
    return TRUE;
}

void skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
    struct jpeg_source_mgr* src = (struct jpeg_source_mgr*) cinfo->src;

    if (num_bytes > 0) {
        src->next_input_byte += (size_t) num_bytes;
        src->bytes_in_buffer -= (size_t) num_bytes;
    }
}

void term_source (j_decompress_ptr cinfo) {}
void jpeg_mem_src2 (j_decompress_ptr cinfo, void* buffer, long nbytes)
{
    struct jpeg_source_mgr* src;

    if (cinfo->src == NULL) {
        cinfo->src = (struct jpeg_source_mgr *)
                (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                            sizeof(struct jpeg_source_mgr));
    }

    src = (struct jpeg_source_mgr*) cinfo->src;
    src->init_source = init_source;
    src->fill_input_buffer = fill_input_buffer;
    src->skip_input_data = skip_input_data;
    src->resync_to_restart = jpeg_resync_to_restart;
    src->term_source = term_source;
    src->bytes_in_buffer = nbytes;
    src->next_input_byte = (JOCTET*)buffer;
}

//////////////////////////////////////////////////////////////////////////////////////////////

#endif

void convert_row_rgb24_to_rgb565(unsigned char *rgb24, uint16_t *rgb565, int width) {
    for (int i = 0; i < width; i++) {
        uint8_t r = rgb24[i * 3];
        uint8_t g = rgb24[i * 3 + 1];
        uint8_t b = rgb24[i * 3 + 2];

        rgb565[i] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    }
}

struct jpeg_decompression_context {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    unsigned char *row_buffer;
};

struct jpeg_decompression_context *initialize_jpeg_decompression_context(int output_width) {
    struct jpeg_decompression_context *ctx = static_cast<jpeg_decompression_context *>(malloc(
            sizeof(struct jpeg_decompression_context)));
    if (!ctx) return NULL;

    ctx->cinfo.err = jpeg_std_error(&ctx->jerr);
    jpeg_create_decompress(&ctx->cinfo);

    // Assuming the output width is known and consistent
    int row_stride = output_width * 3; // RGB24 needs 3 bytes per pixel
    ctx->row_buffer = (unsigned char *) malloc(row_stride);

    return ctx;
}

// Cleanup the JPEG decompression context and row buffer
void cleanup_jpeg_decompression_context(struct jpeg_decompression_context *ctx) {
    if (!ctx) return;

    jpeg_destroy_decompress(&ctx->cinfo);
    free(ctx->row_buffer);
    free(ctx);
}

// Decode a JPEG frame and convert it to RGB565 using the pre-initialized context
bool decode_jpeg_frame_to_rgb565_with_context(struct jpeg_decompression_context *ctx, unsigned char *frame,
                                              size_t frame_size, uint16_t *output_buffer, int output_width,
                                              int output_height) {

    if (!ctx || !ctx->row_buffer) return false;

    jpeg_memory_src(&ctx->cinfo, frame, static_cast<int>(frame_size));

    jpeg_read_header(&ctx->cinfo, true);

    ctx->cinfo.out_color_space = JCS_RGB;

    ctx->cinfo.dct_method = JDCT_IFAST;
    ctx->cinfo.do_fancy_upsampling = false;
    ctx->cinfo.two_pass_quantize = false;
    ctx->cinfo.dither_mode = JDITHER_ORDERED;
    ctx->cinfo.scale_num = 1;
    ctx->cinfo.scale_denom = 1;

    bool status = jpeg_start_decompress(&ctx->cinfo);
    if (!status)
        printf("jpeg_start_decompress failed\n");

    while (ctx->cinfo.output_scanline < ctx->cinfo.output_height) {
        unsigned char *buffer_array[1] = {ctx->row_buffer};
        jpeg_read_scanlines(&ctx->cinfo, buffer_array, 1);
        convert_row_rgb24_to_rgb565(ctx->row_buffer, output_buffer + ctx->cinfo.output_scanline * output_width,
                                    output_width);
    }
    jpeg_finish_decompress(&ctx->cinfo);

    return true;
}

