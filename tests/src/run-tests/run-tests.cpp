#include <cstdlib>
#include <cassert>
#include <iostream>

#include "image-decoder/image-decoder.hpp"
#include "run-tests/utils.hpp"

int main(int argc, const char** argv)
{
    std::vector<std::filesystem::path> files
    {
        "../../input-images/rgb_8_bits.png",
        "../../input-images/rgb_16_bits.png",
        "../../input-images/rgba_8_bits.png",
        "../../input-images/rgba_16_bits.png"
    };

    for (auto filepath : files)
    {
        image_decoder::ImageDecoder decoder(filepath);
        decoder.swapBytesOrder();
        uint32_t width { decoder.getImageWidth() };
        uint32_t heigth { decoder.getImageHeight() };
        uint8_t bit_depth { decoder.getImageBitDepth() };
        image_decoder::ImageColorType color_type { decoder.getImageColorType() };
        size_t scanline_size { decoder.getImageScanlineSize() };
        size_t scanlines_size { decoder.getImageScanlinesSize() };
        uint8_t number_of_channels { decoder.getImageNumberOfChannels() };
        utils::Bytes raw_data {decoder.getRawDataCopy() };

        std::cout << "file: " << filepath << "\n";
        std::cout << "width: " << width << "\n";
        std::cout << "heigth: " << heigth << "\n";
        std::cout << "bit depth: " << (uint32_t)bit_depth << "\n";
        std::cout << "scanline size: " << scanline_size << "\n";
        std::cout << "scanlines size: " << scanlines_size << "\n";
        std::cout << "bytes per pixel: " << (uint32_t)number_of_channels << "\n\n";

        filepath.replace_extension(".tiff");
        filepath = "../../output-images" / filepath.filename();

        tests::writeTiffImage
        (
            filepath,
            raw_data,
            width, heigth,
            bit_depth,
            number_of_channels
        );
    }

    return EXIT_SUCCESS;
}
