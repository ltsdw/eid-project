#pragma once

#include <filesystem>
#include <cstdint>
#include <tiffio.h>

#include "image-decoder/image-decoder.hpp"
#include "utils/utils.hpp"

namespace tests {

    void writeTiffImage
    (
        const std::filesystem::path& filename,
        utils::Bytes& raw_data,
        uint32_t width,
        uint32_t height,
        uint8_t bit_depth,
        uint8_t number_of_channels
    );

    void setBytesPerPixel(image_decoder::ImageColorType color_type, uint8_t* bytes_per_pixel);
} // tests
