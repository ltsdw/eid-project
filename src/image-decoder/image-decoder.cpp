#include <cstring>
#include <iostream>

#include "image-formats/png-format.hpp"
#include "image-decoder/image-decoder.hpp"

namespace image_decoder
{

ImageDecoder::ImageDecoder(const std::filesystem::path& image_filepath)
{
    if (!std::filesystem::exists(image_filepath))
    {
        std::cerr << "File does not exist: " << image_filepath << '\n';

        std::exit(EXIT_FAILURE);
    }

    if (image_filepath.extension() == ".png")
    {
        loadPNGImage(image_filepath);
    }

    // TODO: Implement the rest of the logic
}

ImageDecoder::~ImageDecoder() = default;
ImageDecoder::ImageDecoder(ImageDecoder&&) = default;
ImageDecoder& ImageDecoder::operator=(ImageDecoder&&) = default;

void ImageDecoder::loadPNGImage(const std::filesystem::path& image_filepath)
{
    m_data = std::make_unique<image_formats::png_format::PNGFormat>(image_filepath);

    if (not std::holds_alternative<png_image_unique_ptr>(m_data))
    {
        std::cerr << "Failed to allocate memory for PNGFormat class.\n";

        std::exit(EXIT_FAILURE);
    }

    m_image_format_type = utils::typings::ImageFormat::PNG_FORMAT_TYPE;
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

utils::typings::CBytes& ImageDecoder::getRawDataConstRef()
{
    if (m_image_format_type == utils::typings::ImageFormat::PNG_FORMAT_TYPE)
    {
        auto image = getPNGVariantData();

        return (*image)->getRawDataConstRef();
    }

    throw std::runtime_error
    (
        "Format not implement: "
        + std::to_string(static_cast<uint8_t>(m_image_format_type))
        + " not implemented.\n"
    );
} // ImageDecoder::getRawDataRef

utils::typings::Bytes ImageDecoder::getRawDataCopy()
{
    if (m_image_format_type == utils::typings::ImageFormat::PNG_FORMAT_TYPE)
    {
        auto image = getPNGVariantData();

        return (*image)->getRawDataCopy();
    }

    throw std::runtime_error
    (
        "Format not implement: "
        + std::to_string(static_cast<uint8_t>(m_image_format_type))
        + " not implemented.\n"
    );
} // ImageDecoder::getRawDataCopy

uint8_t* ImageDecoder::getRawDataBuffer()
{
    if (m_image_format_type == utils::typings::ImageFormat::PNG_FORMAT_TYPE)
    {
        auto image = getPNGVariantData();

        uint8_t* ptr = new uint8_t[(*image)->getImageScanlinesSize()];

        std::memcpy(ptr, (*image)->getRawDataBuffer(), (*image)->getImageScanlinesSize());

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
    if (m_image_format_type == utils::typings::ImageFormat::PNG_FORMAT_TYPE)
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
    if (m_image_format_type == utils::typings::ImageFormat::PNG_FORMAT_TYPE)
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
    if (m_image_format_type == utils::typings::ImageFormat::PNG_FORMAT_TYPE)
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

utils::typings::ImageColorType ImageDecoder::getImageColorType() const
{
    if (m_image_format_type == utils::typings::ImageFormat::PNG_FORMAT_TYPE)
    {
        auto image = getPNGVariantData();

        switch ((*image)->getImageColorType())
        {
            case GRAYSCALE_COLOR_TYPE:
                return utils::typings::ImageColorType::GRAYSCALE_COLOR_TYPE;
            case RGB_COLOR_TYPE:
                return utils::typings::ImageColorType::RGBA_COLOR_TYPE;
            case INDEXED_COLOR_TYPE:
                return utils::typings::ImageColorType::INDEXED_COLOR_TYPE;
            case GRAYSCALE_AND_ALPHA_COLOR_TYPE:
                return utils::typings::ImageColorType::GRAYSCALE_AND_ALPHA_COLOR_TYPE;
            case RGBA_COLOR_TYPE:
                return utils::typings::ImageColorType::RGBA_COLOR_TYPE;
            default:
                throw std::runtime_error
                (
                    std::string("Color type not supported.\n")
                    + __func__ + "\n"
                    + std::to_string((*image)->getImageColorType()) + "\n"
                );
        }
    }

    throw std::runtime_error
    (
        "Format not implement: "
        + std::to_string(static_cast<uint8_t>(m_image_format_type))
        + " not implemented.\n"
    );
} // ImageDecoder::getImageColorType

uint8_t ImageDecoder::getImageNumberOfChannels() const
{
    if (m_image_format_type == utils::typings::ImageFormat::PNG_FORMAT_TYPE)
    {
        auto image = getPNGVariantData();

        return (*image)->getImageNumberOfChannels();
    }

    throw std::runtime_error
    (
        "Format not implement: "
        + std::to_string(static_cast<uint8_t>(m_image_format_type))
        + " not implemented.\n"
    );
} // ImageDecoder::getImageNumberOfChannels

size_t ImageDecoder::getImageScanlineSize() const
{
    if (m_image_format_type == utils::typings::ImageFormat::PNG_FORMAT_TYPE)
    {
        auto image = getPNGVariantData();

        return (*image)->getImageScanlineSize();
    }

    throw std::runtime_error
    (
        "Format not implement: "
        + std::to_string(static_cast<uint8_t>(m_image_format_type))
        + " not implemented.\n"
    );
} // ImageDecoder::getImageScanlineSize

size_t ImageDecoder::getImageScanlinesSize() const
{
    if (m_image_format_type == utils::typings::ImageFormat::PNG_FORMAT_TYPE)
    {
        auto image = getPNGVariantData();

        return (*image)->getImageScanlinesSize();
    }

    throw std::runtime_error
    (
        "Format not implement: "
        + std::to_string(static_cast<uint8_t>(m_image_format_type))
        + " not implemented.\n"
    );
} // ImageDecoder::getImageScanlinesSize


void ImageDecoder::swapBytesOrder()
{
    if (m_image_format_type == utils::typings::ImageFormat::PNG_FORMAT_TYPE)
    {
        auto image = getPNGVariantData();

        (*image)->swapBytesOrder();

        return;
    }
} // ImageDecoder::setUseHostEndianess

} // namespace image_formats
