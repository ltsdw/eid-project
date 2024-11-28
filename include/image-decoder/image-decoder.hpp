#pragma once

#include <filesystem>
#include <variant>

#include "utils/typings.hpp"

namespace image_decoder
{
    class ImageDecoder
    {
        public:
            ImageDecoder(const std::filesystem::path& image_filepath);
            ~ImageDecoder();
            ImageDecoder(ImageDecoder&&);
            ImageDecoder& operator=(ImageDecoder&&);
            ImageDecoder(const ImageDecoder&) = delete;
            ImageDecoder& operator=(const ImageDecoder&) = delete;

        public:
            /*!
             * getRawDataConstRef
             *
             * @return: A const reference for the internal raw data vector,
             * make a copy the vector if the class must go out scope or use getRawDataCopy to get a copy.
            */
            [[nodiscard]] [[deprecated("Use getRawDataCopy instead.")]] utils::typings::CBytes& getRawDataConstRef();

            /*!
             * getRawDataCopy
             *
             * @return: A copy for the internal raw data bytes.
            */
            [[nodiscard]] utils::typings::Bytes getRawDataCopy();

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
            [[nodiscard]] utils::typings::ImageColorType getImageColorType() const;

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
            using png_image_unique_ptr = std::unique_ptr<utils::typings::PNGFormat>;

            static constexpr uint8_t GRAYSCALE_COLOR_TYPE { 0 };
            static constexpr uint8_t RGB_COLOR_TYPE { 2 };
            static constexpr uint8_t INDEXED_COLOR_TYPE { 3 };
            static constexpr uint8_t GRAYSCALE_AND_ALPHA_COLOR_TYPE { 4 };
            static constexpr uint8_t RGBA_COLOR_TYPE { 6 };

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
            utils::typings::ImageFormat m_image_format_type;
    }; // class ImageDecoder
}
