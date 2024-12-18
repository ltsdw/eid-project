#pragma once

#include <filesystem>
#include <cstdint>
#include <tiffio.h>

#include "utils/typings.hpp"

namespace tests
{
    void writeTiffImage
    (
        const std::filesystem::path& filename,
        utils::typings::Bytes& raw_data,
        uint32_t width,
        uint32_t height,
        uint8_t bit_depth,
        uint8_t number_of_channels
    );
} // namespace tests
