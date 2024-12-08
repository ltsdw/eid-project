#include <cassert>
#include <iostream>
#include <bit>

#include "run-tests/utils.hpp"

namespace tests
{

/*!
 * TODO: Also use in-house implementation in the future
*/
void writeTiffImage
(
    const std::filesystem::path& filename,
    utils::typings::Bytes& raw_data,
    uint32_t width,
    uint32_t height,
    uint8_t bit_depth,
    uint8_t number_of_channels
)
{
    TIFF* tif = TIFFOpen(filename.c_str(), "w");

    if (!tif)
    {
        std::cerr << "Failed to open TIFF file for writing!" << std::endl;
        return;
    }

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bit_depth);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, number_of_channels);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, width * number_of_channels));
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

    const uint8_t bytes_per_pixel = bit_depth * number_of_channels / 8;

    for (int row = 0; row < height; ++row)
    {
        TIFFWriteScanline(tif, std::bit_cast<void*>(&raw_data[row * width * bytes_per_pixel]), row, 0);
    }

    TIFFClose(tif);
    std::cout << "TIFF image written to " << filename << std::endl;
} // writeTiffImage
} // namespace tests
