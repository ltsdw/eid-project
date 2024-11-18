#include <iostream>
#include "image-decoder/image-decoder.hpp"

namespace image_formats
{

ImageDecoder::ImageDecoder(const std::filesystem::path& image_filepath)
{
    if (image_filepath.extension() == ".png")
    {
        loadPNGImage(image_filepath);
    }

    // TODO: Implement the rest of the logic
}

void ImageDecoder::loadPNGImage(const std::filesystem::path& image_filepath)
{
    m_data = std::make_unique<png_format::PNGFormat>(image_filepath);

    if (not std::holds_alternative<png_image_unique_ptr>(m_data))
    {
        std::cout << "Failed to allocate memory for png image.\n";
        std::exit(EXIT_FAILURE);
    }

    auto& png_image = std::get<png_image_unique_ptr>(m_data);

    if (not png_image)
    {
        std::cout << "Failed to allocate memory for png image.\n";
        std::exit(EXIT_FAILURE);
    }
} // loadPNGImage

} // namespace image_formats
