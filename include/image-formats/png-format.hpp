#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>

#include "abstract-image-formats/abstract-image-formats.hpp"

namespace image_formats::png_format
{
/*!
 * Mainly https://en.wikipedia.org/wiki/PNG and http://www.libpng.org/pub/png/spec/1.2/
 * were used as documentation for creating this library.
 *
 * This library also is a lot simpler than a full implementation like https://github.com/nothings/stb/blob/master/stb_image.h
 * and focus on being a educational material on handling png files.
 *
 * Also only critical chunks handling will be implemented.
 *
 * The steps for handling a png file goes as follows:
 *
 * - First we need to read the first 8 bytes of data, which contains the png signature,
 * based on this signature we can tell if the binary we're reading is a png file or not.
 *      - The 4 bytes that follow have the length of the chunk. (this value is in network-byte-order).
 *      - The other 4 bytes tells the type of the chunk.
 *      - This fild have "length" size and it's the chunk data.
 *      - Finally 4 bytes for the CRC which we use to check for errors in the image data we fetched.
 *      - The CRC is calculated by applying the CRC-32 algorithm to the entire chunk data and type
 *        excluding the length, after calculating we can use to check against the CRC the image gave us.
 *
 *      ---------------------------------------------------------------------
 *      | 8 bytes             | 4 bytes       | 4 bytes           | 4 bytes |
 *      --------------------------------------------------------------------
 *      |  png signature      | chunk length  | type of the chunk | CRC     |
 *      |  0x89504E470D0A1A0A |               |                   |         |
 *      ---------------------------------------------------------------------
 *
 * - After reading and fetching all of that, the first critical (chunks we MUST support, otherwise we can't correctly read images)
 * chunk we have to handle is the IHDR (Image Header) chunk, IHDR have 13 bytes of data
 * (if we check the length size we fetched in the step before we can confirm it):
 *
 *      -----------------------------------------------------------------------------------------------------------------
 *      | 4 bytes | 4 bytes | 1 byte          | 1 byte        | 1 byte             | 1 byte         | 1 byte            |
 *      -----------------------------------------------------------------------------------------------------------------
 *      | width   | height  | bit depth       | color type    | compression method | filter method  | interlaced method |
 *      |         |         | 1, 2, 4, 8, 16  | 0, 2, 3, 4, 6 | always 0           | always 0       | 0 or 1            |
 *      -----------------------------------------------------------------------------------------------------------------
 *
 *      - Color type:
 *          - 0 = grayscale
 *          - 2 = true color (rgb).
 *          - 3 = index-color.
 *          - 4 = grayscale with alpha.
 *          - 6 = true color with alpha (rgba).
 *
 *      - Compression method is always DEFLATE compression algorithm.
 *      - While there's only one filtering method, there's 5 types of filtering:
 *          - 0 = None
 *          - 1 = Sub, is the difference between the byte being processed and the byte before in the same scanline.
 *                To decode we add the byte being processed back to the byte before it.
 *          - 2 = Up, is the difference between the byte being processed and the byte right abobe in the previous scanline.
 *                And to decode we add the byte being processed to the byte right above in the previous scanline.
 *          - 3 = Average, is the difference between the byte being processed
 *                and the average of the byte before in the same scanline and the byte above in the previous scanline.
 *                To decode we take the same average and sum with the byte being processed.
 *          - 4 = Paeth, is the difference of the byte being processed and the paeth predictor,
 *                which uses an algorithm that estimate the value based
 *                on the byte before the byte being processed, the byte right above above in the previous scanline,
 *                and the upper left byte of the above byte.
 *                It selects the byte that is closest to the predicted value.
 *                To decode we calculate the paeth predictor and add it back to the byte being processed.
 *
 * - The next chunk will be PLTE (Palette), IF and only IF the color type is of type 3 (check the IHDR chunk color type),
 * this chunk is important where each pixel value is an index to a palette of colors.
 * if this chunk appears in a non indexed-color image (I don't even know if this is possible), it's safe to ignore it.
 *
 * - The next chunk will be the IDAT (Image Data), it contains the image data,
 * it's composed by multiples other IDAT chunks, we need to concatenate all the IDAT chunks until the IEND is met.
 * The data is compressed,we have to decompress after we fetch all the IDAT chunks so we can have the raw image data.
 *
 * - Finally the IEND (Image End), a 0 byte field indicating the end of the image file.
*/

/*!
 * Scanlines
 *
 * For PNG, scanlines are a straightforward way of representing pixels on screen,
 * each row has size of (image width * image number of channels * the size in bytes of each channel),
 * in total there are "image height" of rows (i.e. a 6x6, truecolor, 8 bits depth color image would have
 * 6 rows of 18 bytes each, 3 channels (red, green, blue) * 6 * 6.
 *
 * There's also the extra byte at the beginning of each scanline which tells what filter was used for the scanline.
 * so in the example above, each scanline would actually have 19 bytes each, but we don't output this byte to the final
 * defiltered data.
 *
*/
class Scanlines
{
private:
    static constexpr uint8_t NONE_FILTER_TYPE {0x0};
    static constexpr uint8_t SUB_FILTER_TYPE {0x1};
    static constexpr uint8_t UP_FILTER_TYPE {0x2};
    static constexpr uint8_t AVERAGE_FILTER_TYPE {0x3};
    static constexpr uint8_t PAETH_FILTER_TYPE {0x4};

    using ScanlineBegin = utils::typings::Bytes::iterator;
    using ScanlineEnd = utils::typings::Bytes::iterator;
    using CScanlineBegin = utils::typings::Bytes::const_iterator;
    using CScanlineEnd = utils::typings::Bytes::const_iterator;

public:
    Scanlines() = default;
    Scanlines(uint32_t scanline_size, uint32_t scanlines_size, uint8_t stride);
    Scanlines(const Scanlines&) = default;
    Scanlines(Scanlines&&) = default;
    Scanlines& operator=(const Scanlines&) = default;
    Scanlines& operator=(Scanlines&&) = default;

public:
    /*!
     * defilterData
     *
     * Applies the necessary filter for each scanline.
     * @param filtered_data: Filtered data to be defiltered.
     * @return
    */
    void defilterData(utils::typings::CBytes& filtered_data, utils::typings::Bytes& defiltered_data);

private:
    void defilterSubFilter(
        CScanlineBegin filtered_scanline_begin,
        CScanlineEnd filtered_scanline_end,
        ScanlineBegin defiltered_scanline_begin
    );
    void defilterUpFilter(
        CScanlineBegin filtered_scanline_begin,
        CScanlineEnd filtered_scanline_end,
        CScanlineBegin previous_defiltered_scanline_begin,
        CScanlineEnd previous_defiltered_scanline_end,
        ScanlineBegin defiltered_scanline_begin
    );
    void defilterAverageFilter
    (
        CScanlineBegin filtered_scanline_begin,
        CScanlineEnd filtered_scanline_end,
        CScanlineBegin previous_defiltered_scanline_begin,
        CScanlineEnd previous_defiltered_scanline_end,
        ScanlineBegin defiltered_scanline_begin
    );
    void defilterPaethFilter
    (
        CScanlineBegin filtered_scanline_begin,
        CScanlineEnd filtered_scanline_end,
        CScanlineBegin previous_defiltered_scanline_begin,
        CScanlineEnd previous_defiltered_scanline_end,
        ScanlineBegin defiltered_scanline_begin
    );
    [[nodiscard]] uint8_t paethPredictor(
        int left_of_current,
        int above_current,
        int upper_left_of_current
    ) const noexcept;
private:
    uint8_t m_stride { 0 };
    utils::typings::Bytes::difference_type m_scanline_size { 0 };
    uint32_t m_scanlines_size { 0 };
}; // Scalines


class PNGFormat : abstract_image_formats::AbstractImageFormats
{
public:
    PNGFormat(const std::filesystem::path& image_filepath);
    ~PNGFormat();
    PNGFormat(PNGFormat&&) = delete;
    PNGFormat(const PNGFormat&) = delete;
    PNGFormat& operator=(const PNGFormat&) = delete;
    PNGFormat& operator=(const PNGFormat&&) = delete;

private:
    static constexpr uint8_t  CHUNK_TYPE_FIELD_BYTES_SIZE    { 4 };
    static constexpr uint8_t  CHUNK_LENGTH_FIELD_BYTES_SIZE  { 4 };
    static constexpr uint8_t  CRC_FIELD_BYTES_SIZE           { 4 };
    static constexpr uint8_t  SIGNATURE_FIELD_BYTES_SIZE     { 8 };
    static constexpr uint8_t  IHDR_CHUNK_BYTES_SIZE          { 13 };
    static constexpr uint16_t PLTE_CHUNK_MAX_SIZE            { 256 * 3 };
    static constexpr uint32_t IHDR_CHUNK_TYPE                { 0x49484452 };

    struct Chunk
    {
        utils::typings::Bytes m_chunk_type{utils::typings::Bytes(CHUNK_TYPE_FIELD_BYTES_SIZE)};
        utils::typings::Bytes m_chunk_data;
    };

    #pragma pack(push, 1)
    struct IHDRChunk
    {
        uint32_t width{0};
        uint32_t height{0};
        uint8_t bit_depth{0};
        uint8_t color_type{0};
        uint8_t compression_method{0};
        uint8_t filter_method{0};
        uint8_t interlaced_method{0};
    };
    #pragma pack(pop)

public:
    /*!
     * AbstractImageFormats class members
    */
    [[nodiscard]] uint32_t getImageScanlinesSize() const noexcept override;
    [[nodiscard]] uint32_t getImageScanlineSize() const noexcept override;
    [[nodiscard]] uint32_t getImageRGBScanlineSize() const noexcept override;
    [[nodiscard]] uint32_t getImageRGBScanlinesSize() const noexcept override;
    [[nodiscard]] uint32_t getImageRGBAScanlineSize() const noexcept override;
    [[nodiscard]] uint32_t getImageRGBAScanlinesSize() const noexcept override;
    [[nodiscard]] uint32_t getImageWidth() const noexcept override;
    [[nodiscard]] uint32_t getImageHeight() const noexcept override;
    [[nodiscard]] uint8_t getImageBitDepth() const noexcept override;
    [[nodiscard]] utils::typings::ImageColorType getImageColorType() const noexcept override;
    [[nodiscard]] uint8_t getImageNumberOfChannels() const override;
    [[nodiscard]] [[deprecated("Use getRawDataCopy instead.")]] utils::typings::CBytes& getRawDataConstRef() noexcept override;
    [[nodiscard]] utils::typings::Bytes getRawDataCopy() noexcept override;
    [[nodiscard]] uint8_t* getRawDataBuffer() noexcept override;
    [[nodiscard]] utils::typings::Bytes getRawDataRGB() noexcept override;
    [[nodiscard]] uint8_t* getRawDataRGBBuffer() noexcept override;
    [[nodiscard]] utils::typings::Bytes getRawDataRGBA() noexcept override;
    [[nodiscard]] uint8_t* getRawDataRGBABuffer() noexcept override;
    void resetCachedData() noexcept override;
    void swapBytesOrder() noexcept override;

private:
    /*!
     * readNBytes
     *
     * @param data: Vector bytes which will be filled with N bytes from the input stream.
     * @param n_bytes: Number of bytes which should be read into data vector.
     * @return
    */
    void readNBytes(utils::typings::Bytes& data, std::streamsize n_bytes);

    /*!
     * readNBytes
     *
     * @param data: Vector bytes which will be filled with N bytes from the input stream.
     * @param n_bytes: Number of bytes which should be read into data vector.
     * @return
    */
    void readNBytes(void* data, std::streamsize n_bytes);

    /*!
     * readNextChunk
     *
     * Each call read a chunk, until it's called to read the IEND,
     * at this point it will alsways return false and no other chunk
     * will be processed.
     *
     * @param chunk: Chunk to be filled with its respective data.
     * @return: It returns false if the chunk being read is IEND.
    */
    bool readNextChunk(Chunk& chunk);

    /*!
     * fillIHDRData
     *
     * Fill each field of IHDR chunk with its respective raw data.
     *
     * @param data: Vector containing data about the IHDR chunk.
     *
     * @return
    */
    void fillIHDRData(utils::typings::CBytes& data);

    /*!
     * fillPLTEData
     *
     * Fill the palette with colors from the PLTE chunk.
     *
     * @param data: Vector containing data about the PLTE chunk.
     *
     * @return
    */
    void fillPLTEData(utils::typings::Bytes& data);

    /*!
     * unpackData
     *
     * If data is less than 8 bits, it will be expanded to exactly 8 bits.
     * If data is greater than 8 bits an exception will be thrown.
     *
     * Unpacks multiple pixels information within a single byte to separate bytes.
     * @param src: Source data.
     * @param dest: Vector where the converted data will be put on.
     * @return
    */
    void unpackData
    (
        utils::typings::CBytes& src,
        utils::typings::Bytes& dest
    ) const;

    /*!
     * convertDataToRGB
     *
     * Convert data from any color type to RGB, if the color has a alpha channel it will be dropped.
     * Each channel will be converted to 8 bits,
     * unless the original data bit depth is 16, in that case each channel will still have 16 bits.
     *
     * @param src: Source data.
     * @param dest: Vector where the converted data will be put on.
     * @return
    */
    void convertDataToRGB
    (
        utils::typings::CBytes& src,
        utils::typings::Bytes& dest
    ) const;

    /*!
     * convertDataToRGB
     *
     * Convert data from any color type to RGBA, if the color doesn't have a alpha channel it will be added.
     * Each channel will be converted to 8 bits,
     * unless the original data bit depth is 16, in that case each channel will still have 16 bits.
     * @param src: Source data.
     * @param dest: Vector where the converted data will be put on.
     * @return
    */
    void convertDataToRGBA
    (
        utils::typings::CBytes& src,
        utils::typings::Bytes& dest
    ) const;

private:
    std::ifstream m_image_stream;
    utils::typings::Bytes m_signature { utils::typings::Bytes(SIGNATURE_FIELD_BYTES_SIZE) };
    utils::typings::Bytes m_palette;
    IHDRChunk m_ihdr {};
    utils::typings::ImageColorType m_color_type { utils::typings::INVALID_COLOR_TYPE };
    uint8_t m_number_of_samples { 0 };
    uint8_t m_number_of_channels { 0 };
    utils::typings::Bytes m_defiltered_data;
    utils::typings::Bytes m_defiltered_data_rgb;
    utils::typings::Bytes m_defiltered_data_rgba;
    Scanlines m_scanlines;
}; // PNGFormat
}; // namespace image_formats::png_format
