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
    uint8_t* image_bit_depth
)
{
    ImageDecoderWrapper* image_decoder_wrapper = new ImageDecoderWrapper;
    image_decoder_wrapper->image_decoder = new image_decoder::ImageDecoder(image_filepath);
    *image_width = image_decoder_wrapper->image_decoder->getImageWidth();
    *image_height = image_decoder_wrapper->image_decoder->getImageHeight();
    *image_color_type = getImageColorType(image_decoder_wrapper->image_decoder->getImageColorType());
    *image_bit_depth = image_decoder_wrapper->image_decoder->getImageBitDepth();

    return image_decoder_wrapper;
}

void destroyDecoderImageInstance(ImageDecoderWrapper* image_decoder_wrapper)
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

uint8_t* getRawDataPtr(ImageDecoderWrapper* image_decoder_wrapper)
{
    if (not image_decoder_wrapper or not image_decoder_wrapper->image_decoder)
    {
        return nullptr;
    }

    return image_decoder_wrapper->image_decoder->getRawDataPtr();
}

void swapBytesOrder(ImageDecoderWrapper* image_decoder_wrapper)
{
    if (not image_decoder_wrapper or not image_decoder_wrapper->image_decoder) { return; }

    image_decoder_wrapper->image_decoder->swapBytesOrder();
}