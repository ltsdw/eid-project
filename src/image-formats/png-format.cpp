#include <iostream>
#include <string>
#include <cmath>

#include "image-formats/png-format.hpp"
#include "utils/utils.hpp"
#include "utils/zlib-stream-manager.hpp"

namespace image_formats::png_format
{

PNGFormat::PNGFormat(const std::filesystem::path& image_filepath)
{
    m_image_stream.exceptions(std::fstream::badbit | std::fstream::failbit);
    m_image_stream.open(image_filepath, std::fstream::binary);

    if (not m_image_stream.is_open())
    {
        std::cerr << "File isn't open, exiting.\n";
        std::exit(EXIT_FAILURE);
    }

    uint32_t width { 0 };
    uint32_t height { 0 };
    uint8_t  stride { 0 };
    utils::ZlibStreamManager z_lib_stream_manager{};
    utils::typings::Bytes decompressed_data;
    readNBytes(m_signature, SIGNATURE_FIELD_BYTES_SIZE);

    // Parses all essential chunks chunks
    while (true)
    {
        Chunk chunk;

        if (not readNextChunk(chunk)) { break; }

        if (utils::matches(chunk.m_chunk_type, "IHDR"))
        {
            fillIHDRData(chunk.m_chunk_data);

            m_color_type =
                (m_ihdr.color_type == 0x0) ? utils::typings::GRAYSCALE_COLOR_TYPE          :
                (m_ihdr.color_type == 0x2) ? utils::typings::RGB_COLOR_TYPE                :
                (m_ihdr.color_type == 0x3) ? utils::typings::INDEXED_COLOR_TYPE            :
                (m_ihdr.color_type == 0x4) ? utils::typings::GRAYSCALE_AND_ALPHA_COLOR_TYPE:
                (m_ihdr.color_type == 0x6) ? utils::typings::RGBA_COLOR_TYPE               :
                throw std::runtime_error
                (
                    __func__
                    + std::string("\nColor type not supported: ")
                    + std::to_string(static_cast<uint32_t>(m_ihdr.color_type)) + "\n"
                );
            m_number_of_samples =
                (m_color_type == utils::typings::GRAYSCALE_COLOR_TYPE)             ? 1 :
                (m_color_type == utils::typings::RGB_COLOR_TYPE)                   ? 3 :
                (m_color_type == utils::typings::INDEXED_COLOR_TYPE)               ? 1 :
                (m_color_type == utils::typings::GRAYSCALE_AND_ALPHA_COLOR_TYPE)   ? 2 :
                                                                                     4 ;
            m_number_of_channels = (m_color_type == utils::typings::INDEXED_COLOR_TYPE) ? 3 :
                                   m_number_of_samples;

            width = getImageWidth();
            height = getImageHeight();
            stride = (m_ihdr.bit_depth * m_number_of_samples + 7) / 8;

            const uint64_t max_scanlines_size
            {
                // (width x height x bytes_per_pixel) + extra_filter_bytes
                getImageScanlinesSize() + height
            };

            /*!
             * Do not support images that exceeds the limit of UINT32_MAX:
            */
            if (max_scanlines_size > UINT32_MAX)
            {
                throw std::runtime_error
                (
                    "The file exceeds the reasonable limits of sanity. Please rethink your life choices."
                );
            }
        } else if (utils::matches(chunk.m_chunk_type, "PLTE"))
        {
            fillPLTEData(chunk.m_chunk_data);
        } else if (utils::matches(chunk.m_chunk_type, "IDAT"))
        {
            /*!
             * We could concatenate all IDAT chunks beforehand and only then
             * decompress all of it at once, but that would have us with an extra
             * buffer, not to mention all the allocations that would come.
             *
             * Processing each IDAT chunk as they come is a better choice here.
            */

            z_lib_stream_manager.decompressData(chunk.m_chunk_data, decompressed_data);
        }
    }

    // Create the scanlines structures to be defiltered
    m_scanlines = Scanlines
    (
        getImageScanlineSize(),
        getImageScanlinesSize(),
        stride
    );

    /*!
     * Defilter each scanline, leaving them in a state where they can be further processed
     * or returned as is after decompression.
    */
    m_scanlines.defilterData(decompressed_data, m_defiltered_data);
} // PNGFormat::PNGFormat

PNGFormat::~PNGFormat()
{
    m_image_stream.close();
} // PNGFormat::~PNGFormat

void PNGFormat::readNBytes(utils::typings::Bytes& data, std::streamsize n_bytes)
{
    m_image_stream.read(
        std::bit_cast<char*>(data.data()),
        n_bytes
    );
} // PNGFormat::readNBytes

void PNGFormat::readNBytes(void* data, std::streamsize n_bytes)
{
    m_image_stream.read(
        std::bit_cast<char*, void*>(data),
        n_bytes
    );
} // PNGFormat::readNBytes

bool PNGFormat::readNextChunk(Chunk& chunk)
{
    uint32_t length { 0 };
    uint32_t crc { 0 };
    uint32_t data_crc { 0 };

    readNBytes(&length, CHUNK_LENGTH_FIELD_BYTES_SIZE);
    readNBytes(chunk.m_chunk_type, CHUNK_TYPE_FIELD_BYTES_SIZE);

    length = utils::convertFromNetworkByteOrder(length);

    if (utils::matches(chunk.m_chunk_type, "IEND")) { return false; }

    chunk.m_chunk_data.resize(length);
    readNBytes(chunk.m_chunk_data, length);
    readNBytes(&crc, CRC_FIELD_BYTES_SIZE);

    crc = utils::convertFromNetworkByteOrder(crc);

    /*!
     * We first calculate the crc of the first 4 bytes (the chunk type)
     * then we use the crc of these 4 bytes to calculate the rest of the bytes,
     * it would be possible to calculate this only once if we appended both vectors togheter
     * but that would be a waste of space.
     *
     * The final_xor_value is 0 at first because we only XOR the crc against 0xFFFFFFFF
     * when all the bytes' crc are already calculated.
    */
    data_crc = utils::calculateCRC32(chunk.m_chunk_type, 0xFFFFFFFF, 0);
    data_crc = utils::calculateCRC32(chunk.m_chunk_data, data_crc);

    std::string string(std::bit_cast<char*>(chunk.m_chunk_type.data()), CHUNK_TYPE_FIELD_BYTES_SIZE);

    if (data_crc != crc)
    {
        throw std::runtime_error("Crc doesn't match, data may be corrupted.\n");
    }

    return true;
} // PNGFormat::readNextChunk

void PNGFormat::fillIHDRData(utils::typings::CBytes& data)
{
    if (not (data.size() == IHDR_CHUNK_BYTES_SIZE))
    {
        throw std::runtime_error("IHDR chunk mismatch size.\n");
    }

    auto begin = data.begin();
    auto end = data.end();

    m_ihdr.width = utils::readAndAdvanceIter<uint32_t>(begin, end);
    m_ihdr.height = utils::readAndAdvanceIter<uint32_t>(begin, end);
    m_ihdr.bit_depth = utils::readAndAdvanceIter<uint8_t>(begin, end);
    m_ihdr.color_type = utils::readAndAdvanceIter<uint8_t>(begin, end);
    m_ihdr.compression_method = utils::readAndAdvanceIter<uint8_t>(begin, end);
    m_ihdr.filter_method = utils::readAndAdvanceIter<uint8_t>(begin, end);
    m_ihdr.interlaced_method = utils::readAndAdvanceIter<uint8_t>(begin, end);
} // PNGFormat::fillIHDRData

void PNGFormat::fillPLTEData(utils::typings::Bytes& data)
{
    if ((data.size() > PLTE_CHUNK_MAX_SIZE))
    {
        throw std::runtime_error
        (
            std::string("PLTE chunk have unsupported size: ")
            + std::to_string(data.size())
            + "\n"
        );
    }

    m_palette = std::move(data);
} // PNGFormat::fillPLTEData

void PNGFormat::unpackData
(
    utils::typings::CBytes& src,
    utils::typings::Bytes& dest
) const
{
    if (m_ihdr.bit_depth > 8)
    {
        throw std::runtime_error
        (
            __func__
            + std::string("\nBit depth is too big to unpack: ")
            + std::to_string(m_ihdr.bit_depth)
            + "\n"
        );
    }

    if (src.empty())
    {
        throw std::runtime_error
        (
            __func__
            + std::string("\nSource data cannot be empty.\n")
        );
    }

    /*!
     * Only indexed color images and grayscale supports less than 8 bit depth,
     * and only indexed color uses three channels, grayscale will always use just one.
    */
    const uint8_t bit_depth { m_ihdr.bit_depth };
    const uint32_t width { getImageWidth() };
    const uint32_t height { getImageHeight() };
    const uint32_t scanline_size { getImageScanlineSize() };
    const uint8_t samples_per_byte = 8 / m_ihdr.bit_depth;
    const double scaling_factor = (255.0 / ((1 << m_ihdr.bit_depth) - 1));
    const uint8_t mask = (1 << m_ihdr.bit_depth) - 1;

    if (m_color_type == utils::typings::INDEXED_COLOR_TYPE)
    {
        dest.reserve(width * height * 3);
    } else
    {
        dest.reserve(width * height);
    }

    /*!
     * When constructing the scanlines above, we had to account for padding bits for the last byte,
     * because is impossible to have a 1/8 of a byte, or 1/2 of a byte,
     * as we may had added padding bits to defilter the scanlines we have to ignore them now,
     * otherwise the image will have more information than necessary.
     *
     * This happens when the (width * bit_depth * number_of_samples) of an image isn't divisible by 8, so for example,
     * an image with 300 width and bit depth of 1, would result in a scanline with size
     * 37 entire bytes and 4 bits (37 bytes and a half), so we have to add padding to scanline to complete 38 bytes
     * to defilter, and when unpacking we must ignore this extra bits.
    */
    for (uint32_t row = 0; row < height; ++row)
    {
        for (uint32_t column = 0; column < width; ++column)
        {
            /*!
             * We are walking each scanline and taking notice of each pixels' bytes indices
             * they will tell to us exactly at which offset we sitting at relative to how far we are in the scanline.
             *
             * The modulus operation on the help with so we never go past the width byte limit,
             * so for example the same hypothetical image above (300 x 10, 1 bit depth):
             *
             * byte_index_0     = 0 = (0 * 38) + (0 / 8);
             * bits_offset_7    = 7 = ((8 - 1) - (0 % 8)) * 1;
             * byte_index_0     = 0 = (0 * 38) + (1 / 8);
             * bits_offset_6    = 6 = ((8 - 1) - (1 % 8)) * 1;
             * ...
             * byte_index_379   = 379   = (9 * 38) + (296 / 8);
             * bits_offset_7    = 7     = ((8 - 1) - (296 % 8)) * 1;
             * byte_index_379   = 379   = (9 * 38) + (297 / 8);
             * bits_offset_6    = 6     = ((8 - 1) - (297 % 8)) * 1;
             * byte_index_379   = 379   = (9 * 38) + (298 / 8);
             * bits_offset_5    = 5     = ((8 - 1) - (298 % 8)) * 1;
             * byte_index_379   = 379   = (9 * 38) + (299 / 8);
             * bits_offset_4    = 4     = ((8 - 1) - (299 % 8)) * 1;
             *
             * We stop at the pixel in the row 10 (9 for our 0-indexed system)
             * and column 300 (299 for our 0-indexed system),
             * which subtracting our samples per byte from the remainder of the position we're in the scanline
             * gives us the exact index of the bit(s) relative to their respective position within the image's width,
             * then we just scale this index by the number of pixels we are working with inside each bytes.
            */
            const uint32_t byte_index = (row * scanline_size) + (column / samples_per_byte);
            const uint32_t bits_offset = (samples_per_byte - 1 - (column % samples_per_byte)) * bit_depth;

            /*!
             * This may be a index for indexed color type, or a color, for grayscale color type.
             *
             * The mask has the width of the bit_set, it correctly isolates just the samples within the byte we want.
            */
            const uint8_t data = static_cast<uint8_t>(src[byte_index] >> bits_offset) & mask;

            if (m_color_type == utils::typings::INDEXED_COLOR_TYPE)
            {
                /*!
                 * The index is relative to colors, and not bytes, as every color inside the palette is in rgb format
                 * it always have three bytes for color (even for grayscale (just two colors) indexed images),
                 * so we must account for it to index the right color/right channel.
                */
                dest.emplace_back(m_palette[data * 3]);         // red
                dest.emplace_back(m_palette[(data * 3) + 1]);   // green
                dest.emplace_back(m_palette[(data * 3) + 2]);   // blue

                continue;
            }

            /*!
             * If the image is not of the indexed type, rest just the grayscale image to unpack.
             *
             * As the bit depth at most 4 which translate at maximum value of 15,
             * we have to scale this 1, 2, 4 bit depth colors back to 8 bit depth.
             *
             * The scaling factor goes as follows:
             *
             * max_8_bit_color = 255
             * n = bit_depth
             * max_value_for_bit_depth = 2ⁿ-1
             * scaling_factor = rounded_up(max_8_bit_color / max_value_for_bit_depth)
            */
            dest.emplace_back(utils::typings::Byte(std::round(data * scaling_factor)));
        }
    }
}

void PNGFormat::convertDataToRGB
(
    utils::typings::CBytes& src,
    utils::typings::Bytes& dest
) const
{
    if (m_color_type == utils::typings::RGB_COLOR_TYPE) { return; }

    const uint8_t bit_depth = m_ihdr.bit_depth == 16 ? 16 : 8;
    const uint32_t width = utils::convertFromNetworkByteOrder(m_ihdr.width);
    const uint32_t height = utils::convertFromNetworkByteOrder(m_ihdr.height);

    if (m_color_type == utils::typings::RGBA_COLOR_TYPE)
    {
        dest.reserve(width * height * (bit_depth / 8) * 3);

        if (m_ihdr.bit_depth == 16)
        {
            for (uint32_t i = 0; i < src.size(); i += 8)
            {
                const uint8_t first_byte_red = static_cast<uint8_t>(src[i]);
                const uint8_t second_byte_red = static_cast<uint8_t>(src[i + 1]);
                const uint8_t first_byte_green = static_cast<uint8_t>(src[i + 2]);
                const uint8_t second_byte_green = static_cast<uint8_t>(src[i + 3]);
                const uint8_t first_byte_blue = static_cast<uint8_t>(src[i + 4]);
                const uint8_t second_byte_blue = static_cast<uint8_t>(src[i + 5]);
                // Alpha skipped

                dest.emplace_back(utils::typings::Byte(first_byte_red));
                dest.emplace_back(utils::typings::Byte(second_byte_red));
                dest.emplace_back(utils::typings::Byte(first_byte_green));
                dest.emplace_back(utils::typings::Byte(second_byte_green));
                dest.emplace_back(utils::typings::Byte(first_byte_blue));
                dest.emplace_back(utils::typings::Byte(second_byte_blue));
            }

            return;
        }

        if (m_ihdr.bit_depth == 8)
        {
            for (uint8_t i = 0; i < src.size(); i += 4)
            {
                const uint8_t red = static_cast<uint8_t>(src[i]);
                const uint8_t green = static_cast<uint8_t>(src[i + 1]);
                const uint8_t blue = static_cast<uint8_t>(src[i + 2]);
                // Alpha skipped

                dest.emplace_back(utils::typings::Byte(red));
                dest.emplace_back(utils::typings::Byte(green));
                dest.emplace_back(utils::typings::Byte(blue));
            }

            return;
        }

        throw std::runtime_error
        (
            __func__
            + std::string("\nBit depth not supported for color type RGBA_COLOR_TYPE: ")
            + std::to_string(static_cast<uint32_t>(m_ihdr.color_type)) + "\n"
        );
    }

    if (m_color_type == utils::typings::INDEXED_COLOR_TYPE)
    {
        unpackData(src, dest);
        return;
    }

    if (m_color_type == utils::typings::GRAYSCALE_COLOR_TYPE)
    {
        if (m_ihdr.bit_depth == 16)
        {
            for (auto it = src.begin(); it != src.end(); it += 2)
            {
                // red
                dest.emplace_back(*it);
                dest.emplace_back(*(it+1));
                // green
                dest.emplace_back(*it);
                dest.emplace_back(*(it+1));
                // blue
                dest.emplace_back(*it);
                dest.emplace_back(*(it+1));
            }

            return;
        }

        if (m_ihdr.bit_depth == 8)
        {
            for (auto byte : src)
            {
                dest.emplace_back(byte); // red
                dest.emplace_back(byte); // green
                dest.emplace_back(byte); // blue
            }

            return;
        }

        utils::typings::Bytes temp_dest;

        unpackData(src, temp_dest);

        for (auto byte : temp_dest)
        {
            dest.emplace_back(byte); // red
            dest.emplace_back(byte); // green
            dest.emplace_back(byte); // blue
        }

        return;
    }

    if (m_color_type == utils::typings::GRAYSCALE_AND_ALPHA_COLOR_TYPE)
    {
        if (m_ihdr.bit_depth == 16)
        {
            for (auto it = src.begin(); it != src.end(); it += 4)
            {
                // red
                dest.emplace_back(*it);
                dest.emplace_back(*(it+1));
                // green
                dest.emplace_back(*it);
                dest.emplace_back(*(it+1));
                // blue
                dest.emplace_back(*it);
                dest.emplace_back(*(it+1));
            }

            return;
        }

        if (m_ihdr.bit_depth == 8)
        {
            for (auto it = src.begin(); it != src.end(); it += 2)
            {
                dest.emplace_back(*it); // red
                dest.emplace_back(*it); // green
                dest.emplace_back(*it); // blue
            }

            return;
        }

        /*!
         * Case the bit depth is less than 8 bits, unpackData will handle it
        */
        utils::typings::Bytes temp_dest;

        unpackData(src, temp_dest);

        for (auto byte : temp_dest)
        {
            dest.emplace_back(byte); // red
            dest.emplace_back(byte); // green
            dest.emplace_back(byte); // blue
        }

        return;
    }

    throw std::runtime_error
    (
        __func__
        + std::string("\nColor type not supported: ")
        + std::to_string(static_cast<uint32_t>(m_ihdr.color_type)) + "\n"
    );
} // PNGFormat::convertDataToRGB

void PNGFormat::convertDataToRGBA
(
    utils::typings::CBytes& src,
    utils::typings::Bytes& dest
) const
{
    if (m_color_type == utils::typings::RGBA_COLOR_TYPE) { return; }

    utils::typings::Bytes temp_dest;
    const uint8_t bit_depth = m_ihdr.bit_depth == 16 ? 16 : 8;

    if (dest.empty())
    {
        const uint32_t width = utils::convertFromNetworkByteOrder(m_ihdr.width);
        const uint32_t height = utils::convertFromNetworkByteOrder(m_ihdr.height);

        /*!
         * width * height * channel_size * four_channels
        */
        dest.reserve(width * height * (bit_depth / 8) * 4);
    }

    /*!
     * Performance wise it would be better to parse the source data here
     * and add the alpha next, but it would be too much boilerplate and error prone
     * to write everything again, so let's use the convertDataToRGB instead.
    */
    convertDataToRGB(src, temp_dest);

    /*!
     * If the data already is in rgb, no conversion is necessary,
     * use the source vector data instead.
    */
    utils::typings::CBytes& rgb_data = (not temp_dest.empty()) ? temp_dest : src;

    /*!
     * Handles the 16 bit depth.
    */
    if (bit_depth == 16)
    {
        for (uint32_t i = 0; i < rgb_data.size(); i += 6)
        {
            // red
            dest.emplace_back(rgb_data[i]);
            dest.emplace_back(rgb_data[i + 1]);
            // green
            dest.emplace_back(rgb_data[i + 2]);
            dest.emplace_back(rgb_data[i + 3]);
            // blue
            dest.emplace_back(rgb_data[i + 4]);
            dest.emplace_back(rgb_data[i + 5]);
            // alpha 0xFFFF
            dest.emplace_back(utils::typings::Byte(0xFF));
            dest.emplace_back(utils::typings::Byte(0xFF));
        }

        return;
    }

    /*!
     * Handles the bit 8 bit depth.
    */
    for (uint32_t i = 0; i < rgb_data.size(); i += 3)
    {
        dest.emplace_back(rgb_data[i]);                 // red
        dest.emplace_back(rgb_data[i + 1]);             // green
        dest.emplace_back(rgb_data[i + 2]);             // blue
        dest.emplace_back(utils::typings::Byte(0xFF));  // alpha 0xFF
    }
} // PNGFormat::convertDataToRGBA

uint32_t PNGFormat::getImageScanlineSize() const noexcept
{
    const uint32_t width = utils::convertFromNetworkByteOrder(m_ihdr.width);

    return ((width * m_ihdr.bit_depth * m_number_of_samples + 7) / 8);
} // PNGFormat::getScanlinesSize

uint32_t PNGFormat::getImageScanlinesSize() const noexcept
{
    const uint32_t height = utils::convertFromNetworkByteOrder(m_ihdr.height);

    return getImageScanlineSize() * height;
} // PNGFormat::getScanlinesSize

uint32_t PNGFormat::getImageRGBScanlineSize() const noexcept
{
    const uint32_t width = utils::convertFromNetworkByteOrder(m_ihdr.width);
    const uint8_t bit_depth = (m_ihdr.bit_depth <= 8) ? 8 : 16;

    return (width * bit_depth * 3 / 8);
} // PNGFormat::getImageRGBScanlineSize

uint32_t PNGFormat::getImageRGBScanlinesSize() const noexcept
{
    const uint32_t height = utils::convertFromNetworkByteOrder(m_ihdr.height);

    return getImageRGBScanlineSize() * height;
} // PNGFormat::getImageRGBScanlineSize

uint32_t PNGFormat::getImageRGBAScanlineSize() const noexcept
{
    const uint32_t width = utils::convertFromNetworkByteOrder(m_ihdr.width);
    const uint8_t bit_depth = (m_ihdr.bit_depth <= 8) ? 8 : 16;

    return (width * bit_depth * 4 / 8);
} // PNGFormat::getImageRGBAScanlineSize

uint32_t PNGFormat::getImageRGBAScanlinesSize() const noexcept
{
    const uint32_t height = utils::convertFromNetworkByteOrder(m_ihdr.height);

    return getImageRGBAScanlineSize() * height;
} // PNGFormat::getImageRGBAScanlineSize


utils::typings::CBytes& PNGFormat::getRawDataConstRef() noexcept
{
    return m_defiltered_data;
} // PNGFormat::getRawDataRef

utils::typings::Bytes PNGFormat::getRawDataCopy() noexcept
{
    return m_defiltered_data;
} // PNGFormat::getRawDataCopy

uint8_t* PNGFormat::getRawDataBuffer() noexcept
{
    return std::bit_cast<uint8_t*>(m_defiltered_data.data());
} // PNGFormat::getRawDataPtr

utils::typings::Bytes PNGFormat::getRawDataRGB() noexcept
{
    if (m_color_type == utils::typings::RGB_COLOR_TYPE)
    {
        return m_defiltered_data;
    }

    if (not m_defiltered_data_rgb.empty())
    {
        return m_defiltered_data_rgb;
    }

    convertDataToRGB(m_defiltered_data, m_defiltered_data_rgb);

    return m_defiltered_data_rgb;
} // PNGFormat::getRawDataRGB

uint8_t* PNGFormat::getRawDataRGBBuffer() noexcept
{
    if (m_color_type == utils::typings::RGB_COLOR_TYPE)
    {
        return std::bit_cast<uint8_t*>(m_defiltered_data.data());
    }

    if (not m_defiltered_data_rgb.empty())
    {
        return std::bit_cast<uint8_t*>(m_defiltered_data_rgb.data());
    }

    convertDataToRGB(m_defiltered_data, m_defiltered_data_rgb);

    return std::bit_cast<uint8_t*>(m_defiltered_data_rgb.data());
} // PNGFormat::getRawDataRGBBuffer

utils::typings::Bytes PNGFormat::getRawDataRGBA() noexcept
{
    if (m_color_type == utils::typings::RGBA_COLOR_TYPE)
    {
        return m_defiltered_data;
    }

    if (not m_defiltered_data_rgba.empty())
    {
        return m_defiltered_data_rgba;
    }

    convertDataToRGBA(m_defiltered_data, m_defiltered_data_rgba);

    return m_defiltered_data_rgba;
} // PNGFormat::getRawDataRGBA

uint8_t* PNGFormat::getRawDataRGBABuffer() noexcept
{
    if (m_color_type == utils::typings::RGBA_COLOR_TYPE)
    {
        return std::bit_cast<uint8_t*>(m_defiltered_data.data());
    }

    if (not m_defiltered_data_rgba.empty())
    {
        return std::bit_cast<uint8_t*>(m_defiltered_data_rgba.data());
    }

    convertDataToRGBA(m_defiltered_data, m_defiltered_data_rgba);

    return std::bit_cast<uint8_t*>(m_defiltered_data_rgba.data());
} // PNGFormat::getRawDataRGBABuffer

uint32_t PNGFormat::getImageWidth() const noexcept
{
    return utils::convertFromNetworkByteOrder(m_ihdr.width);
} // PNGFormat::getImageWidth

uint32_t PNGFormat::getImageHeight() const noexcept
{
    return utils::convertFromNetworkByteOrder(m_ihdr.height);
} // PNGFormat::getImageHeight

uint8_t PNGFormat::getImageBitDepth() const noexcept
{
    return m_ihdr.bit_depth;
} // PNGFormat::getImageBitDepth

utils::typings::ImageColorType PNGFormat::getImageColorType() const noexcept
{
    return m_color_type;
} // PNGFormat::getImageColorType

uint8_t PNGFormat::getImageNumberOfChannels() const
{
    return m_number_of_channels;
} // PNGFormat::getImageNumberOfChannels

void PNGFormat::resetCachedData() noexcept
{
    utils::typings::Bytes ().swap(m_defiltered_data_rgb);
    //m_defiltered_data_rgb.shrink_to_fit();
    utils::typings::Bytes ().swap(m_defiltered_data_rgba);
    //m_defiltered_data_rgba.shrink_to_fit();
} // PNGFormat::resetCachedData

void PNGFormat::swapBytesOrder() noexcept
{
    if (m_ihdr.bit_depth < 16) { return; }

    for (auto it = m_defiltered_data.begin(); it != m_defiltered_data.end(); it += 2)
    {
        std::swap(*it, *(it+1));
    }
} // PNGFormat::swapBytesOrder

Scanlines::Scanlines
(
    uint32_t scanline_size,
    uint32_t scanlines_size,
    uint8_t stride
)
{
    /*!
     * The stride will tell the distance between one byte and the next byte needed to do operations like defiltering.
     * In other words, the byte channel of one pixel, must match the byte of
     * the next pixel it's being processed against, red with red, green with green, and so on.
     *
     * As the filters are applied on a per bytes basis,
     * the strider is calculated the same way, to point to the byte which relates to the one being processed,
     * no matter the bit depth. When there's no relationship of channels
     * for single channels like grayscale, the indices of indexed colors, or less than 8 bit depth images,
     * there's no need to keep relationship of the with concepts like "pixels and channels" anymore,
     * we just defilter normally accounting that they're 1 byte.
     *
     * The + 7 is a way of rounding up to an entire byte, so for bit depths like 1 and 2, it counts as an entire byte,
     * instead of reporting 0 bytes.
    */
    m_stride = stride;
    m_scanline_size = scanline_size;
    m_scanlines_size = scanlines_size;
} // Scalines::Scalines

void Scanlines::defilterData(utils::typings::CBytes& filtered_data, utils::typings::Bytes& defiltered_data)
{
    // Initialize and resize all the space needed to accommodate all scanlines
    defiltered_data.resize(m_scanlines_size);

    const auto it = filtered_data.cbegin();
    const auto it_end = filtered_data.cend();
    const auto it_defiltered = defiltered_data.cbegin();
    const auto it_defiltered_end = defiltered_data.cend();

    /*!
     * As every scanline starts with an extra byte for the filter type, we must
     * sum the size of a scanline plus the extra byte.
    */
    for (uint32_t row = 0; row < filtered_data.size(); row += m_scanline_size + 1)
    {
        const auto extra_filter_bytes_accumulated = (row / (m_scanline_size + 1));
        const auto filtered_scanline_begin = filtered_data.begin() + row + 1;
        const auto filtered_scanline_end = filtered_data.begin() + row + m_scanline_size + 1;
        const auto defiltered_scanline_begin = defiltered_data.begin() + row - extra_filter_bytes_accumulated;

        if (not utils::isWithinBoundaries(it, it_end, filtered_scanline_begin, filtered_scanline_end)
            or not utils::isWithinBoundaries
            (
                it_defiltered,
                it_defiltered_end,
                static_cast<utils::typings::Bytes::const_iterator>(defiltered_scanline_begin)
            )
        ) { throw std::out_of_range(std::string("Out of range iterators: ") + __func__); }

        const auto filter_type = static_cast<uint8_t>(filtered_data[row]);
        auto previous_defiltered_scanline_begin = defiltered_data.cend();
        auto previous_defiltered_scanline_end = defiltered_data.cend();

        // It means we have a previous scanline
        if (row > 0)
        {
            previous_defiltered_scanline_begin = defiltered_scanline_begin - m_scanline_size;
            previous_defiltered_scanline_end = defiltered_scanline_begin + m_scanline_size;
        }

        /*!
         * Filters, differently from those applied from editing image software, serves the purpose
         * increasing the efficiency of compression algorithms, doing so, they reduce
         * the redundancy within and between scanlines.
         *
         * By reducing redundancy, filter allows for better compression which results in smaller file sizes, and the best of all
         * at no loss of information, what goes in, will go out, no loss whatsover.
         * Each scanline has its filter, a combination of all filters is made to better compress the entire image,
         * the result wouldn't be as good if just one filter was chosen for the entire image.
         *
         * Now if we were to encode images instead, how to choose between the available filters?
         * Tipically heuristics algorithms are used to decide,
         * these algorithms won't find the most optmized filter for each scanline,
         * instead they would find something that is "good enough" for efficient compression.
         * While I won’t cover these algorithms here, it’s important to note that the chosen filters are not random,
         * there’s a method to the selection process.
         *
         * Before explaining each filter and how to defilter them, there's some rules we must establish,
         * the filters' formula are static, meaning there are no different treatment when there's
         * "missing" needed value for calculation,
         * for example, when a formula asks us to subtract the current byte being processed from the byte before it,
         * what do we do in the case the current scanline has no previous byte yet to start with?
         * We treat the previous byte as being 0, same applies for the above and upper left byte,
         * if there isn't any, they're 0.
         *
         * Another rule is that there's no negative value when filtering, so each operation which would cause the value
         * to go below zero, gets wrapped around by getting the modulus of 256 (the max number which a byte can hold) of this number,
         * this is only important when encoding, not so much when decoding.
         *
         * The processing happens on a per bytes basis,
         * current_channel_red_byte with previous_channel_red_byte, green with green, blue with blue, and so on,
         * if there's more than one byte (in case of 16 bits depth color) we continue the processing on a per byte basis,
         * works the same for the previous scanline too, we must align the channels before processing.
         *
         * And for a bit depth less than 8 bits, grayscale and indexed color, we treat it all the same, meaning,
         * for a pack of pixels inside a byte, we still process this as a per byte basis, byte by byte,
         * pack of pixels by pack of pixels no different treatment, same for the indices in indexed color and the grayscale,
         * for grayscale with alpha we would still need to align the grayscale channel and the alpha channel.
         *
         * -----------------------------------------------------------------------------------------------------------------
         *
         * None filter, there's nothing to do,
         * we simply leave the filtered scanline as is in the defiltered scanline.
         *
         * -----------------------------------------------------------------------------------------------------------------
         *
         * Sub filter formula:
         *
         * CurrentFilteredByte(x) = CurrentRawByte(x) - PreviousFilteredByte(x)
         *
         * To defilter it, we simply do the inverse operation, instead of subtracting we sum.
         *
         * CurrentDefilteredByte(x) = CurrentFilteredByte(x) + PreviousDefilteredByte(x)
         *
         * ---------
         * |---|---|
         * |PDB|CFB|
         * ---------
         *
         * If there's no byte before the current byte being processed, the byte before is 0,
         * which leave the current filtered byte as is: value + 0 = value.
         * This applies just for the first byte being processed, as the next byte will have a byte before,
         * we must continue processing the scanline to the end.
         *
         * -----------------------------------------------------------------------------------------------------------------
         *
         * Up filter formula:
         *
         * CurrentFilteredByte(x) = CurrentRawByte(x) - AboveFilteredByte(x)
         *
         * To the defilter it, do the inverse operation, we sum.
         *
         * CurrentDefilteredByte(x) = CurrentFilteredByte(x) + AboveDefilteredByte(x)
         *
         * ---------
         * |---|ADB|
         * |---|CFB|
         * ---------
         *
         * This filter depends on the previous scanline to the one being processed,
         * if there's none yet, the entire previous scanline must be treated as a scanline of zeroes,
         * being that the case, the current scanline being processed will be left as is: value + 0 = value
         *
         * -----------------------------------------------------------------------------------------------------------------
         *
         * Average filter formula:
         *
         * CurrentFilteredByte(x) = CurrentRawByte(x) - rounded_down((AboveFilteredByte(x) + PreviousFilteredByte(x) / 2))
         *
         * To defilter, we sum the average instead of subtracting.
         *
         * CurrentDefilteredByte(x) = CurrentFilteredByte(x) + rounded_down((AboveDefilteredByte(x) + PreviousDefilteredByte(x) / 2))
         *
         * ---------
         * |---|ADB|
         * |PDB|CFB|
         * ---------
         *
         * This filter depends on the previous scanline to the current one being processed,
         * if there's none yet, the entire previous scanline must be treated as a scanline of zeroes,
         * and if there's no byte before the one currently being processed it's also 0.
         *
         * -----------------------------------------------------------------------------------------------------------------
         *
         * Paeth filter formula:
         *
         * CurrentFilteredByte(x) =
         * CurrentRawByte(x) - PaethPredictor(PreviousFilteredByte(x), AboveFilteredByte(x), UpperLeftFilteredByte(x))
         *
         * To defilter instead of subtracting the result of the paeth predictor, we sum it.
         *
         * Paeth predictor algorithm pseudo code:
         *
         * InitialPredictor = Previous + Above - UpperLeft
         * PredictorPrevious = InitialPredictor - Previous
         * PredictorAbove = InitialPredictor - Above
         * PredictorUpperLeft = InitialPredictor - UpperLeft
         * FinalPredictor = Previous if PredictorPrevious <= PredictorAbove and PredictorPrevious <= PredictorUpperLeft
         *                  else Above if PredictorAbove <= PredictorUpperLeft
         *                  else UpperLeft
         *
         * This filter depends on the previous scanline to the current one being processed,
         * if there's none yet, the entire previous scanline must be treated as a scanline of zeroes,
         * and if there's no byte before the one currently being processed it's also 0.
         *
         * ---------
         * |UDB|ADB|
         * |PDB|CFB|
         * ---------
        */

        switch (filter_type)
        {
            case NONE_FILTER_TYPE:
                std::copy
                (
                    filtered_scanline_begin,
                    filtered_scanline_end,
                    defiltered_scanline_begin
                );
                break;
            case SUB_FILTER_TYPE:
                defilterSubFilter
                (
                    filtered_scanline_begin,
                    filtered_scanline_end,
                    defiltered_scanline_begin
                );
                break;
            case UP_FILTER_TYPE:
                defilterUpFilter
                (
                    filtered_scanline_begin,
                    filtered_scanline_end,
                    previous_defiltered_scanline_begin,
                    previous_defiltered_scanline_end,
                    defiltered_scanline_begin
                );
                break;
            case AVERAGE_FILTER_TYPE:
                defilterAverageFilter
                (
                    filtered_scanline_begin,
                    filtered_scanline_end,
                    previous_defiltered_scanline_begin,
                    previous_defiltered_scanline_end,
                    defiltered_scanline_begin
                );
                break;
            case PAETH_FILTER_TYPE:
                defilterPaethFilter
                (
                    filtered_scanline_begin,
                    filtered_scanline_end,
                    previous_defiltered_scanline_begin,
                    previous_defiltered_scanline_end,
                    defiltered_scanline_begin
                );
                break;
            default:
                // This should never happen
                throw std::runtime_error("Filter mode is invalid.\n");
                break;
        };
    }
} // Scalines::defilterData

void Scanlines::defilterSubFilter
(
    CScanlineBegin filtered_scanline_begin,
    CScanlineEnd filtered_scanline_end,
    ScanlineBegin defiltered_scanline_begin
)
{
    /*!
     * Remember the rule, if there isn't a previous byte, it's 0,
     * this would be the equivalent of doing (current byte + 0), which would be the current byte itself.
     *
     * So we copy the first pixel as is to our unfiltered scanline.
     *
     * The entire pixel, because we operate on a per byte basis, if there's no byte before to match each channel,
     * we leave all the channels of the first byte as is.
     *
     * For bit depth less than 8 we treat the whole packed pixels as one thing,
     * instead of doing this per channel matching on each bit.
    */
    std::copy
    (
        filtered_scanline_begin,
        filtered_scanline_begin + m_stride,
        defiltered_scanline_begin
    );

    /*!
     * We already processed the "current byte", so point to the start of the "next byte".
     * Remember, each byte must match its equivalent "channel byte" (red with red, green with green, etc),
     * and for packed pixels, or single channel the rule is the same, byte with byte, not bit with bit.
    */
    filtered_scanline_begin += m_stride;
    defiltered_scanline_begin += m_stride;

    for (;filtered_scanline_begin != filtered_scanline_end; ++filtered_scanline_begin, ++defiltered_scanline_begin)
    {
        const auto current = static_cast<uint8_t>(*filtered_scanline_begin);
        const auto left_of_current = static_cast<uint8_t>(*(defiltered_scanline_begin - m_stride));
        *defiltered_scanline_begin = utils::typings::Byte(current + left_of_current);
    }
} // Scalines::defilterSubFilter

void Scanlines::defilterUpFilter(
    CScanlineBegin filtered_scanline_begin,
    CScanlineEnd filtered_scanline_end,
    CScanlineBegin previous_defiltered_scanline_begin,
    CScanlineEnd previous_defiltered_scanline_end,
    ScanlineBegin defiltered_scanline_begin
)
{
    if (previous_defiltered_scanline_begin == previous_defiltered_scanline_end)
    {
        /*!
         * If there's no byte before the current one or a previous scanline the one currently being processed,
         * assume it's 0, which for defiltering the Up filter, is the filtered scanline itself.
         *
         * CurrentDefilteredByte(x) = CurrentFilteredByte(x) + AboveCurrentDefilteredByte(x),
         *
         * AboveCurrentDefilteredPixel(x) will always be 0, so we leave the entire scanline as is.
        */
        std::copy(filtered_scanline_begin, filtered_scanline_end, defiltered_scanline_begin);
        return;
    }

    /*!
     * We have a previous scanline, so the defiltering continues normally.
    */
    for (;filtered_scanline_begin != filtered_scanline_end;
        ++filtered_scanline_begin,
        ++defiltered_scanline_begin,
        ++previous_defiltered_scanline_begin
    )
    {
        const auto current = static_cast<uint8_t>(*filtered_scanline_begin);
        const auto above_current = static_cast<uint8_t>(*(previous_defiltered_scanline_begin));
        *defiltered_scanline_begin = std::byte(current + above_current);
    }
} // Scalines::defilterUpFilter

void Scanlines::defilterAverageFilter
(
    CScanlineBegin filtered_scanline_begin,
    CScanlineEnd filtered_scanline_end,
    CScanlineBegin previous_defiltered_scanline_begin,
    CScanlineEnd previous_defiltered_scanline_end,
    ScanlineBegin defiltered_scanline_begin
)
{
    if (previous_defiltered_scanline_begin == previous_defiltered_scanline_end)
    {
        /*!
         * The formula to defilter average filter is:
         *
         * CurrentDefilteredByte(x) =
         * CurrentFilteredByte(x) + rounded_down(PreviousDefilteredByte(x) + AboveCurrentDefilteredByte(x) / 2)
         *
         * If there's no previous scanline, it's 0, and as we just starting to process this scanline,
         * there isn't any previous pixel yet. The above would expand to:
         *
         * CurrentDefilteredPixel(x) = CurrentFilteredPixel(x) + rounded_down(0 + 0 / 2)
         * CurrentDefilteredPixel(x) = CurrentFilteredPixel(x) + 0
         * CurrentDefilteredPixel(x) = CurrentFilteredPixel(x)
         *
         * In other words we just copy the bytes as is to the current scanline being processed
         * to have a start value.
        */
        std::copy
        (
            filtered_scanline_begin,
            filtered_scanline_begin + m_stride,
            defiltered_scanline_begin
        );

        /*!
         * We already processed the "current byte", so point to the start of the "next byte".
         * Remember, each byte must match its equivalent "channel byte" (red with red, green with green, etc),
         * and for packed pixels, or single channel the rule is the same, byte with byte, not bit with bit.
        */
        filtered_scanline_begin += m_stride;
        defiltered_scanline_begin += m_stride;

        for (;filtered_scanline_begin != filtered_scanline_end; ++filtered_scanline_begin, ++defiltered_scanline_begin)
        {
            const auto current = static_cast<uint8_t>(*filtered_scanline_begin);
            const auto left_of_current = static_cast<uint8_t>(*(defiltered_scanline_begin - m_stride));
            *defiltered_scanline_begin = utils::typings::Byte(current + std::floor(left_of_current / 2));
        }

        return;
    }

    /*!
     * This handles the case where we have a previous scanline, but not a previous byte yet,
     * differently from the Sub filter, we can't simply copy the first bytes as is,
     * the operation doesn't need only the previous byte, but also the above to the average and we have that.
     *
     * So it becomes:
     *
     * CurrentDefilteredByte(x) = CurrentFilteredByte(x) + rounded_down(0 + AboveCurrentDefilteredByte(x) / 2)
     * CurrentDefilteredByte(x) = CurrentFilteredByte(x) + rounded_down(AboveDefilteredByte(x) / 2)
     *
    */

    auto initial_stride_end = filtered_scanline_begin + m_stride;

    for (
        ;filtered_scanline_begin != initial_stride_end;
        ++filtered_scanline_begin,
        ++defiltered_scanline_begin,
        ++previous_defiltered_scanline_begin
    )
    {
        const auto current = static_cast<uint8_t>(*filtered_scanline_begin);
        const auto above_current = static_cast<uint8_t>(*previous_defiltered_scanline_begin);
        *defiltered_scanline_begin = utils::typings::Byte(current + std::floor(above_current / 2));
    }

    /*!
     * The iterators above are already pointing to the right byte to be processed.
    */
    for (
        ;filtered_scanline_begin != filtered_scanline_end;
        ++filtered_scanline_begin,
        ++defiltered_scanline_begin,
        ++previous_defiltered_scanline_begin
    )
    {
        const auto current = static_cast<uint8_t>(*filtered_scanline_begin);
        const auto left_of_current = static_cast<uint8_t>(*(defiltered_scanline_begin - m_stride));
        const auto above_current = static_cast<uint8_t>(*previous_defiltered_scanline_begin);
        *defiltered_scanline_begin = utils::typings::Byte(current + std::floor((left_of_current + above_current) / 2));
    }
} // Scanlines::defilterAverageFilter

void Scanlines::defilterPaethFilter
(
    CScanlineBegin filtered_scanline_begin,
    CScanlineEnd filtered_scanline_end,
    CScanlineBegin previous_defiltered_scanline_begin,
    CScanlineEnd previous_defiltered_scanline_end,
    ScanlineBegin defiltered_scanline_begin
)
{
    /*!
     * The paeth filter formula goes as:
     *
     * CurrentFilteredByte(x) =
     * CurrentRawByte(x) - PaethPredictor(PreviousFilteredByte(x), AboveFilteredByte(x), UpperLeftFilteredByte(x))
     *
     * The formula to revert the paeth filter is:
     *
     * CurrentDefilteredByte(x) =
     * CurrentFilteredByte(x) + PaethPredictor(PreviousDefilteredByte(x), AboveCurrentDefilteredByte(x), UpperLeftDefilteredByte(x))
    */

    /*!
     * When there's no previous scanline, there won't be above or upper left bytes, they will all be 0, the property of the
     * paeth predictor is that it will return the non-zero value provided to it, as long as the other two values are 0.
     * In the case above, where there's no previous scanline,
     * it would always return the value of the byte before the one being processed.
    */
    if (previous_defiltered_scanline_begin == previous_defiltered_scanline_end)
    {
        /*!
         * Here is as if the left, above and upper left were 0, if we passed all zeroes to the paeth predictor,
         * it would return 0, summing the current filtered byte to zero would result in the filtered byte itself,
         * so we copy it as is.
        */
        std::copy(filtered_scanline_begin, filtered_scanline_begin + m_stride, defiltered_scanline_begin);

        filtered_scanline_begin += m_stride;
        defiltered_scanline_begin += m_stride;

        for (
            ;filtered_scanline_begin != filtered_scanline_end;
            ++filtered_scanline_begin,
            ++defiltered_scanline_begin
        )
        {
            const auto current = static_cast<uint8_t>(*filtered_scanline_begin);
            const auto left_of_current = static_cast<uint8_t>(*(defiltered_scanline_begin - m_stride));
            *defiltered_scanline_begin = utils::typings::Byte(current + left_of_current);
        }

        return;
    }

    /*!
     * This is the case where we have a previous scanline, but as we are at the beginning of it,
     * there's no previous byte for the current filtered byte and there's no upper left byte for the current filtered byte,
     * but there's a defiltered byte above the current filtered byte.
     *
     * Passing this above byte to the paeth predictor, and zero for the other values, would return the above byte itself,
     * so we just sum the current filtered byte to this defiltered byte above.
    */
    auto current_pixel_end = filtered_scanline_begin + m_stride;

    for (
        ;filtered_scanline_begin != current_pixel_end;
        ++filtered_scanline_begin,
        ++previous_defiltered_scanline_begin,
        ++defiltered_scanline_begin
    ){
        const auto current = static_cast<uint8_t>(*filtered_scanline_begin);
        const auto above_current = static_cast<uint8_t>(*previous_defiltered_scanline_begin);
        *defiltered_scanline_begin = utils::typings::Byte(current + above_current);
    }

    /*!
     * All edge cases handled, we continue the defilter as normal.
    */
    for (
        ;filtered_scanline_begin != filtered_scanline_end;
        ++filtered_scanline_begin,
        ++previous_defiltered_scanline_begin,
        ++defiltered_scanline_begin
    )
    {
        const auto current = static_cast<uint8_t>(*filtered_scanline_begin);
        const auto left_of_current = static_cast<uint8_t>(*(defiltered_scanline_begin - m_stride));
        const auto above_current = static_cast<uint8_t>(*previous_defiltered_scanline_begin);
        const auto upper_left_of_current = static_cast<uint8_t>(*(previous_defiltered_scanline_begin - m_stride));
        *defiltered_scanline_begin = utils::typings::Byte
        (
            current + paethPredictor
            (
                left_of_current,
                above_current,
                upper_left_of_current
            )
        );
    }
} // Scanlines::defilterPaethFilter


uint8_t Scanlines::paethPredictor
(
    int left_of_current,
    int above_current,
    int upper_left_of_current
) const noexcept
{
    const int p = left_of_current + above_current - upper_left_of_current;
    const int p_left_of_current = std::abs(p - left_of_current);
    const int p_above_current = std::abs(p - above_current);
    const int p_upper_left_of_current = std::abs(p - upper_left_of_current);

    if (p_left_of_current <= p_above_current and p_left_of_current <= p_upper_left_of_current)
        return left_of_current;
    else if (p_above_current <= p_upper_left_of_current)
        return above_current;
    else
        return upper_left_of_current;
} // Scalines::paethPredictor
} // namespace image_formats::png_format
