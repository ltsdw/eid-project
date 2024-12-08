#pragma once

#include <filesystem>
#include <variant>

#include "abstract-image-formats/abstract-image-formats.hpp"

namespace image_decoder
{
class ImageDecoder : abstract_image_formats::AbstractImageFormats
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
     * AbstractImageFormats class members
    */
    [[nodiscard]] [[deprecated("Use getRawDataCopy instead.")]] utils::typings::CBytes& getRawDataConstRef() override;
    [[nodiscard]] utils::typings::Bytes getRawDataCopy() override;
    [[nodiscard]] uint8_t* getRawDataBuffer() override;
    [[nodiscard]] utils::typings::Bytes getRawDataRGB() override;
    [[nodiscard]] uint8_t* getRawDataRGBBuffer() override;
    [[nodiscard]] utils::typings::Bytes getRawDataRGBA() override;
    [[nodiscard]] uint8_t* getRawDataRGBABuffer() override;
    [[nodiscard]] uint32_t getImageWidth() const override;
    [[nodiscard]] uint32_t getImageHeight() const override;
    [[nodiscard]] utils::typings::ImageColorType getImageColorType() const override;
    [[nodiscard]] uint8_t getImageBitDepth() const override;
    [[nodiscard]] uint8_t getImageNumberOfChannels() const override;
    [[nodiscard]] uint32_t getImageScanlineSize() const override;
    [[nodiscard]] uint32_t getImageScanlinesSize() const override;
    [[nodiscard]] uint32_t getImageRGBScanlineSize() const override;
    [[nodiscard]] uint32_t getImageRGBScanlinesSize() const override;
    [[nodiscard]] uint32_t getImageRGBAScanlineSize() const override;
    [[nodiscard]] uint32_t getImageRGBAScanlinesSize() const override;
    void resetCachedData() noexcept override;
    void swapBytesOrder() override;

private:
    using png_image_unique_ptr = std::unique_ptr<utils::typings::PNGFormat>;

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
