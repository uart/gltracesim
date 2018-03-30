#include <cstdio>
#include <cstdint>
#include <cassert>
#include <jpeglib.h>

namespace gltracesim {

void
dump_jpeg_image_wrapper(
    const char *filename, uint8_t *data, size_t width, size_t height)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    //
    FILE *outfile = NULL;

    //
    outfile = fopen(filename, "wb");
    //
    assert(outfile);

    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 7, true);
    jpeg_start_compress(&cinfo, true);

    JSAMPROW row_pointer[1];
    int row_stride = width * 3;

    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &data[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    //
    jpeg_finish_compress(&cinfo);
    //
    fclose(outfile);
    //
    jpeg_destroy_compress(&cinfo);
}

}
