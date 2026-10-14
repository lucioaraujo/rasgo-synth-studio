/* Standalone codec linkage smoke, no GUI, audio or synthesis. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <juce_graphics/image_formats/jpglib/jpeglib.h>
#include <juce_graphics/image_formats/pnglib/png.h>

static void fatal_error(j_common_ptr unused)
{
    (void) unused;
    exit(2);
}

int main(void)
{
    struct jpeg_decompress_struct decoder = {0};
    struct jpeg_error_mgr error = {0};
    /* JUCE supplies its own error callbacks, not IJG's jpeg_std_error unit. */
    error.error_exit = fatal_error;
    decoder.err = &error;
    jpeg_CreateDecompress(&decoder, JPEG_LIB_VERSION, sizeof(decoder));
    jpeg_destroy_decompress(&decoder);
    if (png_get_libpng_ver(NULL) == NULL)
        return 1;
    printf("JPEG %d / PNG %s: pinned codec units linked\n", JPEG_LIB_VERSION, png_get_libpng_ver(NULL));
    return 0;
}
