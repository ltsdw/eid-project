#pragma once

#include <cstdint>

#include "utils/typings.hpp"

namespace abstract_image_formats
{
class AbstractImageFormats
{
public:
    virtual ~AbstractImageFormats() = default;

    /*!
     * Some utilities for exposing some Scanline class member functions.
    */

    /*!
     * getImageScanlinesSize
     *
     * @return: The size of all the scanlines.
    */
    [[nodiscard]] virtual uint32_t getImageScanlinesSize() const = 0;

    /*!
     * getImageScanlineSize
     *
     * @return: The size of a single scanline.
    */
    [[nodiscard]] virtual uint32_t getImageScanlineSize() const = 0;

    /*!
     * getImageRGBScanlineSize
     *
     * This utility serves the purpose of getting the scanline size of images in rgb format,
     * usefull to know the scanlines size when the image has any other color type originally.
     *
     * @return: The size of a single scanline for the image with three channels (red, green, blue).
    */
    [[nodiscard]] virtual uint32_t getImageRGBScanlineSize() const = 0;

    /*!
     * getImageRGBScanlinesSize
     *
     * This utility serves the purpose of getting the scanline size of images in rgb format,
     * usefull to know the scanlines size when the image has any other color type originally.
     *
     * @return: The size of the scanlines for the image with three channels (red, green, blue).
    */
    [[nodiscard]] virtual uint32_t getImageRGBScanlinesSize() const = 0;

    /*!
     * getImageRGBAScanlineSize
     *
     * This utility serves the purpose of getting the scanline size of images in rgb format,
     * usefull to know size of a single scanline when the image has any other color type originally.
     *
     * @return: The size of a single scanline for the image with four channels (red, green, blue, alpha).
    */
    [[nodiscard]] virtual uint32_t getImageRGBAScanlineSize() const = 0;

    /*!
     * getImageRGBAScanlinesSize
     *
     * This utility serves the purpose of getting the scanline size of images in rgb format,
     * usefull to know the scanlines size when the image has any other color type originally.
     *
     * @return: The size of the scanlines for the image with four channels (red, green, blue, alpha).
    */
    [[nodiscard]] virtual uint32_t getImageRGBAScanlinesSize() const = 0;

    /*!
     * getImageWidth
     *
     * @return: Image's width.
    */
    [[nodiscard]] virtual uint32_t getImageWidth() const = 0;

    /*!
     * getImageHeight
     *
     * @return: Image's height.
    */
    [[nodiscard]] virtual uint32_t getImageHeight() const = 0;

    /*!
     * getImageBitDepth
     *
     * @return: Image's bit depth.
    */
    [[nodiscard]] virtual uint8_t getImageBitDepth() const = 0;

    /*!
     * getImageColorType
     *
     * @return: Image's color type.
    */
    [[nodiscard]] virtual utils::typings::ImageColorType getImageColorType() const = 0;

    /*!
     * getNumberOfChannels
     *
     * @return: Image's number of channels.
    */
    [[nodiscard]] virtual uint8_t getImageNumberOfChannels() const = 0;

    /*!
     * getRawDataConstRef
     *
     * @return: A const reference for the internal defiltered vector bytes,
     * make a copy the vector if the class must go out scope or use getRawDataCopy to get a copy.
    */
    [[nodiscard]] [[deprecated("Use getRawDataCopy instead.")]] virtual utils::typings::CBytes& getRawDataConstRef() = 0;

    /*!
     * getRawDataCopy
     *
     * @return: A copy for the internal defiltered vector bytes.
    */
    [[nodiscard]] virtual utils::typings::Bytes getRawDataCopy() = 0;

    /*!
     * getRawDataBuffer
     *
     * @return: A const pointer for the internal defiltered vector bytes,
    */
    [[nodiscard]] virtual uint8_t* getRawDataBuffer() = 0;

    /*!
     * getRawDataRGB
     *
     * @return: A copy for the internal raw data bytes using three channels (red, green, blue).
    */
    [[nodiscard]] virtual utils::typings::Bytes getRawDataRGB() = 0;

    /*!
     * getRawDataRGBBuffer
     *
     * This function will allocate memory and copy the content of the internal vector to the new allocated buffer,
     * using three channels (red, green, blue).
     * the caller must deallocate memory using delete[].
     *
     * @return: A pointer to the allocated memory and copied content of the internal raw data vector
     * using three channels (red, green, blue).
     * It's the caller responsability to deallocate this memory with delete[].
    */
    [[nodiscard]] virtual uint8_t* getRawDataRGBBuffer() = 0;

    /*!
     * getRawDataRGBA
     *
     * @return: A copy for the internal raw data bytes using 4 channels (red, green, blue, alpha).
    */
    [[nodiscard]] virtual utils::typings::Bytes getRawDataRGBA() = 0;

    /*!
     * getRawDataRGBABuffer
     *
     * This function will allocate memory and copy the content of the internal vector to the new allocated buffer,
     * using four channels (red, green, blue, alpha).
     * the caller must deallocate memory using delete[].
     *
     * @return: A pointer to the allocated memory and copied content of the internal raw data vector
     * using four channels (red, green, blue, alpha).
     * It's the caller responsability to deallocate this memory with delete[].
    */
    [[nodiscard]] virtual uint8_t* getRawDataRGBABuffer() = 0;

    /*!
     * resetCachedData
     *
     * Empties all cached data freeing its resources back to the system.
     * Original data won't be touched, leaving it in a usable state.
     *
     * NOTE: For classes that don't use internal cache for conversion this function is noop.
     *
     * @return
    */
    virtual void resetCachedData() = 0;

    /*!
     * swapBytesOrder
     *
     * If image's bit depth > 8 bits, raw data will be reodered, meaning if it's LSB it'll become MSB,
     * and if it's MSB it'll become LSB.
     * If image's bit depth is less than 8 bits, nothing is done.
     *
     * @return
    */
    virtual void swapBytesOrder() = 0;
}; // class AbstractImageFormats
} // namespace abstract_formats
