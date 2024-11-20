#include <cassert>
#include <iostream>
#include <bit>

#include "utils.hpp"

namespace tests
{

/*!
 * TODO: Also use in-house implementation in the future
*/
void writeTiffImage
(
    const std::filesystem::path& filename,
    utils::Bytes& raw_data,
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
}

void setBytesPerPixel(image_decoder::ImageColorType color_type, uint8_t* bytes_per_pixel)
{
    switch (color_type)
    {
        case image_decoder::ImageColorType::GRAYSCALE:
            *bytes_per_pixel = 1;
            break;
        case image_decoder::ImageColorType::GRAYSCALE_ALPHA:
            *bytes_per_pixel = 2;
            break;
        case image_decoder::ImageColorType::RGB:
            *bytes_per_pixel = 3;
            break;
        case image_decoder::ImageColorType::RGBA:
            *bytes_per_pixel = 4;
            break;
        default:
            assert(false and "Color type not supported.\n");
    };
}

} // namespace tests
