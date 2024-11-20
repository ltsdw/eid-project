#include <cstdlib>
#include <cassert>
#include <iostream>

#include "utils.hpp"

int main(int argc, const char** argv)
{
    std::vector<std::filesystem::path> files
    {
        "../input-images/rgb_8_bits.png",
        "../input-images/rgb_16_bits.png",
        "../input-images/rgba_8_bits.png",
        "../input-images/rgba_16_bits.png"
    };

    for (auto filepath : files)
    {
        image_decoder::ImageDecoder decoder(filepath);
        decoder.swapBytesOrder();
        uint32_t widht { decoder.getImageWidth() };
        uint32_t heigth { decoder.getImageHeight() };
        uint8_t bit_depth { decoder.getImageBitDepth() };
        image_decoder::ImageColorType color_type { decoder.getImageColorType() };
        uint8_t bytes_per_pixel { 0 };

        tests::setBytesPerPixel(color_type, &bytes_per_pixel);

        std::cout << "file: " << filepath << "\n";
        std::cout << "width: " << widht << "\n";
        std::cout << "heigth: " << heigth << "\n";
        std::cout << "bit depth: " << (uint32_t)bit_depth << "\n";
        std::cout << "bytes per pixel: " << (uint32_t)bytes_per_pixel << "\n\n";

        filepath.replace_extension(".tiff");
        filepath = "../output-images" / filepath.filename();

        tests::writeTiffImage
        (
            filepath,
            decoder.getRawDataRef(),
            widht, heigth,
            bit_depth,
            bytes_per_pixel
        );
    }

    return EXIT_SUCCESS;
}
