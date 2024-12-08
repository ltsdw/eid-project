#include "image-decoder/image-decoder.hpp"
#include "image-decoder-wrapper/image-decoder-wrapper.h"

struct ImageDecoderWrapper
{
    image_decoder::ImageDecoder* image_decoder;
};

ImageDecoderWrapper* createImageDecoderInstance
(
    const char* image_filepath,
    uint32_t* image_width,
    uint32_t* image_height,
    ImageColorType* image_color_type,
    uint8_t* image_bit_depth,
    uint8_t* image_number_of_channels,
    uint32_t* image_scanline_size,
    uint32_t* image_scanlines_size,
    uint32_t* image_rgb_scanline_size,
    uint32_t* image_rgb_scanlines_size,
    uint32_t* image_rgba_scanline_size,
    uint32_t* image_rgba_scanlines_size,
    const char** error
)
{
    ImageDecoderWrapper* image_decoder_wrapper = nullptr;

    try
    {
        image_decoder_wrapper = new ImageDecoderWrapper;
        image_decoder_wrapper->image_decoder = new image_decoder::ImageDecoder(image_filepath);

        if (image_width)
        {
            *image_width = image_decoder_wrapper->image_decoder->getImageWidth();
        }

        if (image_height)
        {
            *image_height = image_decoder_wrapper->image_decoder->getImageHeight();
        }

        if (image_bit_depth)
        {
            *image_bit_depth = image_decoder_wrapper->image_decoder->getImageBitDepth();
        }

        if (image_number_of_channels)
        {
            *image_number_of_channels = image_decoder_wrapper->image_decoder->getImageNumberOfChannels();
        }

        if (image_scanline_size)
        {
            *image_scanline_size = image_decoder_wrapper->image_decoder->getImageScanlineSize();
        }

        if (image_scanlines_size)
        {
            *image_scanlines_size = image_decoder_wrapper->image_decoder->getImageScanlinesSize();
        }

        if (image_rgb_scanline_size)
        {
            *image_rgb_scanline_size = image_decoder_wrapper->image_decoder->getImageRGBScanlineSize();
        }

        if (image_rgb_scanlines_size)
        {
            *image_rgb_scanlines_size = image_decoder_wrapper->image_decoder->getImageRGBScanlinesSize();
        }

        if (image_rgba_scanline_size)
        {
            *image_rgba_scanline_size = image_decoder_wrapper->image_decoder->getImageRGBAScanlineSize();
        }

        if (image_rgba_scanlines_size)
        {
            *image_rgba_scanlines_size = image_decoder_wrapper->image_decoder->getImageRGBAScanlinesSize();
        }

        if (image_color_type)
        {
            switch (image_decoder_wrapper->image_decoder->getImageColorType())
            {
                case utils::typings::GRAYSCALE_COLOR_TYPE:
                {
                    *image_color_type = GRAYSCALE_COLOR_TYPE;

                    break;
                }
                case utils::typings::RGB_COLOR_TYPE:
                {
                    *image_color_type = RGB_COLOR_TYPE;

                    break;
                }
                case utils::typings::INDEXED_COLOR_TYPE:
                {
                    *image_color_type = INDEXED_COLOR_TYPE;

                    break;
                }
                case utils::typings::RGBA_COLOR_TYPE:
                {
                    *image_color_type = RGBA_COLOR_TYPE;

                    break;
                }
                case utils::typings::GRAYSCALE_AND_ALPHA_COLOR_TYPE:
                {
                    *image_color_type = GRAYSCALE_COLOR_TYPE;

                    break;
                }
                default:
                {
                    throw std::runtime_error(__func__ + std::string("\nInvalid color type.\n"));
                }
            }
        }
    } catch (const std::exception& e)
    {
        *error = e.what();

        destroyImageDecoderInstance(image_decoder_wrapper);

        return nullptr;
    }

    return image_decoder_wrapper;
} // createImageDecoderInstance

void destroyImageDecoderInstance(ImageDecoderWrapper* image_decoder_wrapper)
{
    if (image_decoder_wrapper)
    {
        if (image_decoder_wrapper->image_decoder)
        {
            delete image_decoder_wrapper->image_decoder;
        }

        delete image_decoder_wrapper;
    }
} // destroyImageDecoderInstance

uint8_t* getRawDataBuffer(ImageDecoderWrapper* image_decoder_wrapper, const char** error)
{
    if (not image_decoder_wrapper or not image_decoder_wrapper->image_decoder)
    {
        *error = "Error: Null pointer to ImageDecoder instance, nothing was done.";
        return nullptr;
    }

    try
    {
        return image_decoder_wrapper->image_decoder->getRawDataBuffer();
    } catch (const std::exception& e)
    {
        *error = e.what();
        return nullptr;
    }
} // getRawDataBuffer

uint8_t* getRawDataRGBBuffer(ImageDecoderWrapper* image_decoder_wrapper, const char** error)
{
    if (not image_decoder_wrapper or not image_decoder_wrapper->image_decoder)
    {
        *error = "Error: Null pointer to ImageDecoder instance, nothing was done.";
        return nullptr;
    }

    try
    {
        return image_decoder_wrapper->image_decoder->getRawDataRGBBuffer();
    } catch (const std::exception& e)
    {
        *error = e.what();
        return nullptr;
    }
} // getRawDataRGBBuffer

uint8_t* getRawDataRGBABuffer(ImageDecoderWrapper* image_decoder_wrapper, const char** error)
{
    if (not image_decoder_wrapper or not image_decoder_wrapper->image_decoder)
    {
        *error = "Error: Null pointer to ImageDecoder instance, nothing was done.";
        return nullptr;
    }

    try
    {
        return image_decoder_wrapper->image_decoder->getRawDataRGBABuffer();
    } catch (const std::exception& e)
    {
        *error = e.what();
        return nullptr;
    }
} // getRawDataRGBABuffer

void freeRawDataBuffer(uint8_t* buffer)
{
    if (buffer)
    {
        delete[] buffer;
    }
} // freeRawDataBuffer

int swapBytesOrder(ImageDecoderWrapper* image_decoder_wrapper, const char** error)
{
    if (not image_decoder_wrapper or not image_decoder_wrapper->image_decoder)
    {
        *error = "Error: Null pointer to ImageDecoder instance, nothing was done.";
        return INVALID_ARGUMENTS;
    }

    try
    {
        uint8_t a = image_decoder_wrapper->image_decoder->getImageBitDepth();
        image_decoder_wrapper->image_decoder->swapBytesOrder();
    } catch (const std::exception& e)
    {
        *error = e.what();
        return EXCEPTION;
    }

    return SUCCESS;
} // swapBytesOrder

void resetCachedData(ImageDecoderWrapper* image_decoder_wrapper, const char** error)
{
    if (not image_decoder_wrapper or not image_decoder_wrapper->image_decoder)
    {
        *error = "Error: Null pointer to ImageDecoder instance, nothing was done.";
    }

    try
    {
        image_decoder_wrapper->image_decoder->resetCachedData();
    } catch (const std::exception& e)
    {
        *error = e.what();
    }
} // resetCachedData
