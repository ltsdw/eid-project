#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>

#include "utils/utils.hpp"

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

            using ScanlineBegin = utils::Bytes::iterator;
            using ScanlineEnd = utils::Bytes::iterator;
            using CScanlineBegin = utils::Bytes::const_iterator;
            using CScanlineEnd = utils::Bytes::const_iterator;

        public:
            Scanlines() = default;
            Scanlines(uint32_t width, uint32_t height, uint8_t bit_depth, uint8_t color_type, uint8_t number_of_channels);
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
            void defilterData(utils::CBytes& filtered_data, utils::Bytes& defiltered_data);

            /*!
             * getScanlineSize
             *
             * @return: The size of a single scanline.
            */
            [[nodiscard]] size_t getScanlineSize() const noexcept;

            /*!
             * getScanlinesSize
             *
             * @return: The size of all the scanlines.
            */
            [[nodiscard]] size_t getScanlinesSize() const noexcept;

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
            uint8_t m_number_of_channels { 0 };
            utils::Bytes::difference_type m_scanline_size { 0 };
            size_t m_scanlines_size { 0 };
    }; // Scalines


    class PNGFormat
    {
        public:
            PNGFormat(const std::filesystem::path& image_filepath);
            ~PNGFormat();
            PNGFormat(PNGFormat&&) = delete;
            PNGFormat(const PNGFormat&) = delete;
            PNGFormat& operator=(const PNGFormat&) = delete;
            PNGFormat& operator=(const PNGFormat&&) = delete;

        private:
           static constexpr uint8_t CHUNK_TYPE_FIELD_BYTES_SIZE { 4 };
           static constexpr uint8_t CHUNK_LENGTH_FIELD_BYTES_SIZE { 4 };
           static constexpr uint8_t CRC_FIELD_BYTES_SIZE { 4 };
           static constexpr uint8_t SIGNATURE_FIELD_BYTES_SIZE { 8 };
           static constexpr uint8_t IHDR_CHUNK_BYTES_SIZE { 13 };
           static constexpr uint32_t IHDR_CHUNK_TYPE { 0x49484452 };

           struct Chunk
           {
               utils::Bytes m_chunk_type{utils::Bytes(CHUNK_TYPE_FIELD_BYTES_SIZE)};
               utils::Bytes m_chunk_data;
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
             * Some utilities for exposing some Scanline class member functions.
            */

            /*!
             * getImageScanlinesSize
             *
             * @return: The size of all the scanlines.
            */
            [[nodiscard]] size_t getImageScanlinesSize() const noexcept;

            /*!
             * getImageScanlineSize
             *
             * @return: The size of a single scanline.
            */
            [[nodiscard]] size_t getImageScanlineSize() const noexcept;

            /*!
             * getImageWidth
             *
             * @return: Image's width.
            */
            [[nodiscard]] uint32_t getImageWidth() const noexcept;

            /*!
             * getImageHeight
             *
             * @return: Image's height.
            */
            [[nodiscard]] uint32_t getImageHeight() const noexcept;

            /*!
             * getImageBitDepth
             *
             * @return: Image's bit depth.
            */
            [[nodiscard]] uint8_t getImageBitDepth() const noexcept;

            /*!
             * getImageColorType
             *
             * @return: Image's color type.
            */
            [[nodiscard]] uint8_t getImageColorType() const noexcept;

            /*!
             * getNumberOfChannels
             *
             * @return: Image's number of channels.
            */
            [[nodiscard]] uint8_t getImageNumberOfChannels() const;

            /*!
             * getRawDataConstRef
             *
             * @return: A const reference for the internal defiltered vector bytes,
             * make a copy the vector if the class must go out scope or use getRawDataCopy to get a copy.
            */
            [[nodiscard]] [[deprecated("Use getRawDataCopy instead.")]] utils::CBytes& getRawDataConstRef() noexcept;

            /*!
             * getRawDataCopy
             *
             * @return: A copy for the internal defiltered vector bytes.
            */
            [[nodiscard]] utils::Bytes getRawDataCopy() noexcept;

            /*!
             * getRawDataBuffer
             *
             * @return: A const pointer for the internal defiltered vector bytes,
            */
            [[nodiscard]] const uint8_t* getRawDataBuffer();

            /*!
             * swapBytesOrder
             *
             * If image's bit depth > 8 bits, raw data will be reoder, meaning if it's LSB it'll become MSB,
             * and if it's MSB it'll become MSB.
             * If image's bit depth is less than 8 bits, nothing is done.
             *
             * @return
            */
            void swapBytesOrder();

        private:
            /*!
             * readNBytes
             *
             * @param data: Vector bytes which will be filled with N bytes from the input stream.
             * @param n_bytes: Number of bytes which should be read into data vector.
             * @return
            */
            void readNBytes(utils::Bytes& data, std::streamsize n_bytes);

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
             * @return
            */
            void fillIHDRData(utils::CBytes& data);

        private:
            std::ifstream m_image_stream;
            utils::Bytes m_signature { utils::Bytes(SIGNATURE_FIELD_BYTES_SIZE) };
            IHDRChunk m_ihdr {};
            uint8_t m_number_of_channels { 0 };
            Scanlines m_scanlines;
            utils::Bytes m_defiltered_data;
    }; // PNGFormat
}; // namespace image_formats::png_format
