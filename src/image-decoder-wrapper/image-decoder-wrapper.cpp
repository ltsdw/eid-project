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
    size_t* image_scanline_size,
    size_t* image_scanlines_size,
    const char** error
)
{
    ImageDecoderWrapper* image_decoder_wrapper = nullptr;

    try
    {
        image_decoder_wrapper = new ImageDecoderWrapper;
        image_decoder_wrapper->image_decoder = new image_decoder::ImageDecoder(image_filepath);
        *image_width = image_decoder_wrapper->image_decoder->getImageWidth();
        *image_height = image_decoder_wrapper->image_decoder->getImageHeight();
        *image_bit_depth = image_decoder_wrapper->image_decoder->getImageBitDepth();
        *image_number_of_channels = image_decoder_wrapper->image_decoder->getImageNumberOfChannels();
        *image_scanline_size = image_decoder_wrapper->image_decoder->getImageScanlineSize();
        *image_scanlines_size = image_decoder_wrapper->image_decoder->getImageScanlinesSize();

        switch (image_decoder_wrapper->image_decoder->getImageColorType())
        {
            case utils::typings::GRAYSCALE_COLOR_TYPE:
                *image_color_type = GRAYSCALE_COLOR_TYPE;
            case utils::typings::RGB_COLOR_TYPE:
                *image_color_type = RGB_COLOR_TYPE;
            case utils::typings::INDEXED_COLOR_TYPE:
                *image_color_type = INDEXED_COLOR_TYPE;
            case utils::typings::RGBA_COLOR_TYPE:
                *image_color_type = RGBA_COLOR_TYPE;
            case utils::typings::GRAYSCALE_AND_ALPHA_COLOR_TYPE:
                *image_color_type = GRAYSCALE_COLOR_TYPE;
            default:
                throw std::runtime_error(__func__ + std::string("\nInvalid color type.\n"));
        }
    } catch (const std::exception& e)
    {
        *error = e.what();

        destroyImageDecoderInstance(image_decoder_wrapper);
    }

    return image_decoder_wrapper;
}

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
}

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
}

void freeRawDataBuffer(uint8_t* buffer)
{
    if (buffer)
    {
        delete[] buffer;
    }
}

int swapBytesOrder(ImageDecoderWrapper* image_decoder_wrapper, const char** error)
{
    if (not image_decoder_wrapper or not image_decoder_wrapper->image_decoder)
    {
        *error = "Error: Null pointer to ImageDecoder instance, nothing was done.";
        return INVALID_ARGUMENTS;
    }

    try
    {
        image_decoder_wrapper->image_decoder->swapBytesOrder();
    } catch (const std::exception& e)
    {
        *error = e.what();
        return EXCEPTION;
    }

    return SUCCESS;
}
