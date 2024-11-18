#pragma once

#include <filesystem>
#include <variant>

#include "image-formats/png-format.hpp"

namespace image_formats
{
    class ImageDecoder
    {
        public:
            using png_image_unique_ptr = std::unique_ptr<png_format::PNGFormat>;

        public:
            ImageDecoder(const std::filesystem::path& image_filepath);
            ~ImageDecoder() = default;
            ImageDecoder(ImageDecoder&&) = delete;
            ImageDecoder(const ImageDecoder&) = delete;
            ImageDecoder& operator=(ImageDecoder&&) = delete;
            ImageDecoder& operator=(const ImageDecoder&) = delete;

        private:
            void loadPNGImage(const std::filesystem::path& image_filepath);

        private:
            std::variant<png_image_unique_ptr> m_data;
    };
}
