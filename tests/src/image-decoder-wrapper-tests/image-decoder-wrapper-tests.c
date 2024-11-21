#include <stdio.h>
#include <stdlib.h>

#include "image-decoder-wrapper/image-decoder-wrapper.h"

int main(int argc, const char** argv)
{
    uint32_t width = 0;
    uint32_t height = 0;
    ImageColorType image_color_type;
    uint8_t image_bit_depth = 0;

    ImageDecoderWrapper* image_decoder_wrapper =
    createImageDecoderInstance
    (
        "../../input-images/rgb_8_bits.png",
        &width,
        &height,
        &image_color_type,
        &image_bit_depth
    );

    printf("Image width: %d\n", width);
    printf("Image height: %d\n", height);
    printf("Image color type: %d\n", image_color_type);
    printf("Image bit depth: %d\n", image_bit_depth);

    return EXIT_SUCCESS;
}
