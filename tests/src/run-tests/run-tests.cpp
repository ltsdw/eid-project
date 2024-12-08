#include <cstdlib>
#include <cassert>
#include <iostream>

#include "image-decoder/image-decoder.hpp"
#include "run-tests/utils.hpp"
#include "utils/typings.hpp"

int main(int argc, const char** argv)
{
    std::vector<std::filesystem::path> files
    {
        "../../input-images/indexed_1_bit_depth.png",
        "../../input-images/indexed_2_bit_depth.png",
        "../../input-images/indexed_4_bit_depth.png",
        "../../input-images/indexed_8_bit_depth.png",
        "../../input-images/grayscale_1_bit_depth.png",
        "../../input-images/grayscale_2_bit_depth.png",
        "../../input-images/grayscale_4_bit_depth.png",
        "../../input-images/grayscale_8_bit_depth.png",
        "../../input-images/grayscale_16_bit_depth.png",
        "../../input-images/rgb_8_bit_depth.png",
        "../../input-images/rgb_16_bit_depth.png",
        "../../input-images/rgba_8_bit_depth.png",
        "../../input-images/rgba_16_bit_depth.png"
    };

    for (auto filepath : files)
    {
#ifdef DEBUG_ALLOCATOR
        utils::typings::BytesAllocator::enableLogging();
#endif

        image_decoder::ImageDecoder decoder(filepath);
        decoder.swapBytesOrder();
        uint32_t width { decoder.getImageWidth() };
        uint32_t heigth { decoder.getImageHeight() };
        uint8_t bit_depth { decoder.getImageBitDepth() };
        utils::typings::ImageColorType color_type { decoder.getImageColorType() };
        uint32_t scanline_size { decoder.getImageScanlineSize() };
        uint32_t scanlines_size { decoder.getImageScanlinesSize() };
        uint8_t number_of_channels { decoder.getImageNumberOfChannels() };

        utils::typings::Bytes raw_data { decoder.getRawDataRGBA() };
        decoder.resetCachedData();

#ifdef DEBUG_ALLOCATOR
        utils::typings::BytesAllocator::disableLogging();
#endif

        std::cout << "file: " << filepath << "\n";
        std::cout << "width: " << width << "\n";
        std::cout << "heigth: " << heigth << "\n";
        std::cout << "bit depth: " << (uint32_t)bit_depth << "\n";
        std::cout << "scanline size: " << scanline_size << "\n";
        std::cout << "scanlines size: " << scanlines_size << "\n";
        std::cout << "number of channels: " << (uint32_t)number_of_channels << "\n\n";
        std::cout << "color type: " << (int)(color_type) << "\n";

        filepath.replace_extension(".tiff");
        filepath = "../../output-images" / filepath.filename();

        tests::writeTiffImage
        (
            filepath,
            raw_data,
            width, heigth,
            (bit_depth <= 8) ? 8 : 16,
            4
        );

        std::cout << "------------------------------------------\n";
    }

    return EXIT_SUCCESS;
}
