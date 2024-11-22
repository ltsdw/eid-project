#include "image-decoder/image-decoder.hpp"
#include "image-decoder-wrapper/image-decoder-wrapper.h"

struct ImageDecoderWrapper
{
    image_decoder::ImageDecoder* image_decoder;
};


static ImageColorType getImageColorType(image_decoder::ImageColorType image_color_type)
{
    switch (image_color_type)
    {
        case image_decoder::ImageColorType::GRAYSCALE:
                return GRAYSCALE;
        case image_decoder::ImageColorType::RGB:
            return RGB;
        //TODO: Support indexed palette images
        case image_decoder::ImageColorType::GRAYSCALE_ALPHA:
            return GRAYSCALE_ALPHA;
        case image_decoder::ImageColorType::RGBA:
            return RGBA;
        default:
            return INVALID_COLOR_TYPE;
    }
}

ImageDecoderWrapper* createImageDecoderInstance
(
    const char* image_filepath,
    uint32_t* image_width,
    uint32_t* image_height,
    ImageColorType* image_color_type,
    uint8_t* image_bit_depth,
    uint8_t* image_number_of_channels,
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
        *image_color_type = getImageColorType(image_decoder_wrapper->image_decoder->getImageColorType());
        *image_bit_depth = image_decoder_wrapper->image_decoder->getImageBitDepth();
        *image_number_of_channels = image_decoder_wrapper->image_decoder->getImageNumberOfChannels();
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
