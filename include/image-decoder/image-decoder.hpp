#pragma once

#include <filesystem>
#include <variant>

#include "image-formats/png-format.hpp"

namespace image_decoder
{
    /*!
     * Some types and type aliases for easy of documentation.
    */

    // TODO: Create string representation of the enums.
    enum class ImageFormat
    {
        PNG_FORMAT_TYPE = 0x1,
    }; // enum class ImageFormat

    enum class ImageColorType
    {
        GRAYSCALE           = 0x1,
        GRAYSCALE_ALPHA     = 0x2,
        RGB                 = 0x3,
        RGBA                = 0x4,
    }; // enum class ImageColorType

    class ImageDecoder
    {
        public:
            ImageDecoder(const std::filesystem::path& image_filepath);
            ~ImageDecoder() = default;
            ImageDecoder(ImageDecoder&&) = default;
            ImageDecoder(const ImageDecoder&) = delete;
            ImageDecoder& operator=(ImageDecoder&&) = default;
            ImageDecoder& operator=(const ImageDecoder&) = delete;

        public:
            /*!
             * getRawDataConstRef
             *
             * @return: A const reference for the internal raw data vector,
             * make a copy the vector if the class must go out scope or use getRawDataCopy to get a copy.
            */
            [[nodiscard]] [[deprecated("Use getRawDataCopy instead.")]] utils::CBytes& getRawDataConstRef();

            /*!
             * getRawDataCopy
             *
             * @return: A copy for the internal raw data bytes.
            */
            [[nodiscard]] utils::Bytes getRawDataCopy();

            /*!
             * getRawDataBuffer
             *
             * This function will allocate memory and copy the content of the internal vector to the new allocated buffer.
             * The caller must deallocate memory using delete[].
             *
             * @return: A pointer to the allocated memory and copied content of the internal raw data vector.
             * It's the caller responsability to deallocate this memory with delete[].
            */
            [[nodiscard]] uint8_t* getRawDataBuffer();

            /*!
             * getImageWidth
             *
             * @return: Image's width.
            */
            [[nodiscard]] uint32_t getImageWidth() const;

            /*!
             * getImageHeight
             *
             * @return: Image's height.
            */
            [[nodiscard]] uint32_t getImageHeight() const;

            /*!
             * getImageColorType
             *
             * @return: Image's color type.
            */
            [[nodiscard]] ImageColorType getImageColorType() const;

            /*!
             * getImageBitDepth
             *
             * @return: Image's bit depth.
            */
            [[nodiscard]] uint8_t getImageBitDepth() const;

            /*!
             * getNumberOfChannels
             *
             * @return: Image's number of channels.
            */
            [[nodiscard]] uint8_t getImageNumberOfChannels() const;

            /*!
             * getImageScanlineSize
             *
             * @return: The size of a single scanline.
            */
            [[nodiscard]] size_t getImageScanlineSize() const;

            /*!
             * getImageScanlinesSize
             *
             * @return: The size of all the scanlines.
            */
            [[nodiscard]] size_t getImageScanlinesSize() const;

            /*!
             * swapBytesOrder
             *
             * If bit depth > 8 bits, raw data will be reoder, meaning if it's LSB it'll become MSB,
             * and if it's MSB it'll become MSB.
             *
             * @return
            */
            void swapBytesOrder();

        private:
            using png_image_unique_ptr = std::unique_ptr<image_formats::png_format::PNGFormat>;

        private:
            /*!
             * loadPNGImage
             *
             * @param image_filepath: Image filepath.
             * @return
            */
            void loadPNGImage(const std::filesystem::path& image_filepath);

            // TODO: Load more formats

            /*!
             * getPNGVariantData
             *
             * @return: If m_data holds a png unique_ptr, it will return it, failing to do so,
             * it will terminate the program.
            */
            [[nodiscard]] png_image_unique_ptr* getPNGVariantData() noexcept;
            [[nodiscard]] const png_image_unique_ptr* getPNGVariantData() const noexcept;

        private:
            std::variant<png_image_unique_ptr> m_data;
            ImageFormat m_image_format_type;
    }; // class ImageDecoder
}
