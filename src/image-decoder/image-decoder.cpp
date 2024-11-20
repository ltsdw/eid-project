#include <iostream>

#include "image-decoder/image-decoder.hpp"

namespace image_decoder
{

ImageDecoder::ImageDecoder(const std::filesystem::path& image_filepath)
{
    if (!std::filesystem::exists(image_filepath))
    {
        std::cerr << "Error: File does not exist: " << image_filepath << '\n';

        std::exit(EXIT_FAILURE);
    }

    if (image_filepath.extension() == ".png")
    {
        loadPNGImage(image_filepath);
    }

    // TODO: Implement the rest of the logic
}

void ImageDecoder::loadPNGImage(const std::filesystem::path& image_filepath)
{
    m_data = std::make_unique<image_formats::png_format::PNGFormat>(image_filepath);

    if (not std::holds_alternative<png_image_unique_ptr>(m_data))
    {
        std::cerr << "Failed to allocate memory for PNGFormat class.\n";

        std::exit(EXIT_FAILURE);
    }

    m_image_format_type = ImageFormat::PNG_FORMAT_TYPE;
} // ImageDecoder::loadPNGImage

ImageDecoder::png_image_unique_ptr* ImageDecoder::getPNGVariantData() noexcept
{
    auto image = std::get_if<png_image_unique_ptr>(&m_data);

    if (not image)
    {
        std::cerr << "Variant data isn't holding a std::unique_ptr<PNGFormat>.\n";

        std::exit(EXIT_FAILURE);
    }

    return image;
} // ImageDecoder::getPNGVariantData

const ImageDecoder::png_image_unique_ptr* ImageDecoder::getPNGVariantData() const noexcept
{
    auto image = std::get_if<png_image_unique_ptr>(&m_data);

    if (not image)
    {
        std::cerr << "Variant data isn't holding a std::unique_ptr<PNGFormat>.\n";

        std::exit(EXIT_FAILURE);
    }

    return image;
} // ImageDecoder::getPNGVariantData

utils::Bytes& ImageDecoder::getRawDataRef()
{
    if (m_image_format_type == ImageFormat::PNG_FORMAT_TYPE)
    {
        auto image = getPNGVariantData();

        return (*image)->getRawDataRef();
    }

    throw std::runtime_error
    (
        "Format not implement: "
        + std::to_string(static_cast<uint8_t>(m_image_format_type))
        + " not implemented.\n"
    );
} // ImageDecoder::getRawDataRef

utils::Bytes ImageDecoder::getRawDataCopy()
{
    if (m_image_format_type == ImageFormat::PNG_FORMAT_TYPE)
    {
        auto image = getPNGVariantData();

        return (*image)->getRawDataRef();
    }

    throw std::runtime_error
    (
        "Format not implement: "
        + std::to_string(static_cast<uint8_t>(m_image_format_type))
        + " not implemented.\n"
    );
} // ImageDecoder::getRawDataCopy

uint8_t* ImageDecoder::getRawDataPtr()
{
    if (m_image_format_type == ImageFormat::PNG_FORMAT_TYPE)
    {
        auto image = getPNGVariantData();

        uint8_t* ptr = new uint8_t[(*image)->getScanlinesSize()];

        std::memcpy(ptr, (*image)->getRawDataPtr(), (*image)->getScanlinesSize());

        return ptr;
    }

    throw std::runtime_error
    (
        "Format not implement: "
        + std::to_string(static_cast<uint8_t>(m_image_format_type))
        + " not implemented.\n"
    );
} // ImageDecoder::getRawDataPtr

uint32_t ImageDecoder::getImageWidth() const
{
    if (m_image_format_type == ImageFormat::PNG_FORMAT_TYPE)
    {
        auto image = getPNGVariantData();

        return (*image)->getImageWidth();
    }

    throw std::runtime_error
    (
        "Format not implement: "
        + std::to_string(static_cast<uint8_t>(m_image_format_type))
        + " not implemented.\n"
    );

} // ImageDecoder::getImageWidth

uint32_t ImageDecoder::getImageHeight() const
{
    if (m_image_format_type == ImageFormat::PNG_FORMAT_TYPE)
    {
        auto image = getPNGVariantData();

        return (*image)->getImageHeight();
    }

    throw std::runtime_error
    (
        "Format not implement: "
        + std::to_string(static_cast<uint8_t>(m_image_format_type))
        + " not implemented.\n"
    );
} // ImageDecoder::getImageHeight

uint8_t ImageDecoder::getImageBitDepth() const
{
    if (m_image_format_type == ImageFormat::PNG_FORMAT_TYPE)
    {
        auto image = getPNGVariantData();

        return (*image)->getImageBitDepth();
    }

    throw std::runtime_error
    (
        "Format not implement: "
        + std::to_string(static_cast<uint8_t>(m_image_format_type))
        + " not implemented.\n"
    );
} // ImageDecoder::getImageBitDepth

ImageColorType ImageDecoder::getImageColorType() const
{
    if (m_image_format_type == ImageFormat::PNG_FORMAT_TYPE)
    {
        auto image = getPNGVariantData();

        switch ((*image)->getImageColorType())
        {
            case 0x0:
                return ImageColorType::GRAYSCALE;
            case 0x2:
                return ImageColorType::RGB;
            //TODO: Support indexed palette images
            case 0x4:
                return ImageColorType::GRAYSCALE_ALPHA;
            case 0x6:
                return ImageColorType::RGBA;
            default:
                throw std::runtime_error(__func__ + std::string("\nColor type not supported.\n"));
        }
    }

    throw std::runtime_error
    (
        "Format not implement: "
        + std::to_string(static_cast<uint8_t>(m_image_format_type))
        + " not implemented.\n"
    );

} // ImageDecoder::getImageWidth

void ImageDecoder::swapBytesOrder()
{
    if (m_image_format_type == ImageFormat::PNG_FORMAT_TYPE)
    {
        auto image = getPNGVariantData();

        (*image)->swapBytesOrder();

        return;
    }
} // ImageDecoder::setUseHostEndianess

} // namespace image_formats
