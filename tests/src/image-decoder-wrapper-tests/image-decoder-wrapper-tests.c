#include <stdio.h>
#include <stdlib.h>

#include "image-decoder-wrapper/image-decoder-wrapper.h"

int main(int argc, const char** argv)
{
    uint32_t width = 0;
    uint32_t height = 0;
    ImageColorType image_color_type;
    uint8_t image_bit_depth = 0;
    uint8_t image_number_of_channels = 0;
    uint32_t image_scanline_size = 0;
    uint32_t image_scanlines_size = 0;
    uint32_t image_rgb_scanline_size = 0;
    uint32_t image_rgb_scanlines_size = 0;
    uint32_t image_rgba_scanline_size = 0;
    uint32_t image_rgba_scanlines_size = 0;

    const char* error = NULL;

    ImageDecoderWrapper* image_decoder_wrapper =
    createImageDecoderInstance
    (
        "../../input-images/indexed_1_bit_depth.png",
        &width,
        &height,
        &image_color_type,
        &image_bit_depth,
        &image_number_of_channels,
        &image_scanline_size,
        &image_scanlines_size,
        &image_rgb_scanline_size,
        &image_rgb_scanlines_size,
        &image_rgba_scanline_size,
        &image_rgba_scanlines_size,
        &error
    );


    if (! image_decoder_wrapper)
    {
        printf("createImageDecoderInstance failed: %s\n", error);

        return EXIT_FAILURE;
    }

    int ret = swapBytesOrder(image_decoder_wrapper, &error);

    if (ret != 0)
    {
        printf("swapBytesOrder failed: %s\n", error);

        return EXIT_FAILURE;
    }

    uint8_t* raw_data = getRawDataBuffer(image_decoder_wrapper, &error);

    if (! raw_data)
    {
        printf("getRawDataBuffer failed: %s\n", error);

        return EXIT_FAILURE;
    }

    printf("Image width: %d\n", width);
    printf("Image height: %d\n", height);
    printf("Image color type: %d\n", image_color_type);
    printf("Image bit depth: %d\n", image_bit_depth);
    printf("Image number of channels: %d\n", image_number_of_channels);
    printf("Image scanline size: %d\n", image_scanline_size);
    printf("Image scanlines size: %d\n", image_scanlines_size);

    freeRawDataBuffer(raw_data);
    destroyImageDecoderInstance(image_decoder_wrapper);

    return EXIT_SUCCESS;
}
