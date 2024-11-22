#ifndef IMAGE_DECODER_WRAPPER_H
#define IMAGE_DECODER_WRAPPER_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

typedef enum
{
    INVALID_COLOR_TYPE  = -0x1,
    GRAYSCALE           = 0x1,
    GRAYSCALE_ALPHA     = 0x2,
    RGB                 = 0x3,
    RGBA                = 0x4,
} ImageColorType; // enum ImageColorType


/*!
 * ImageDecoderWrapper
 *
 * A wrapper around the ImageDecoder class, this will provide a way
 * to use the class using C.
*/
typedef struct ImageDecoderWrapper ImageDecoderWrapper;

/*!
 * Some functions providing a bridge between the member functions of the ImageDecoder
 * and C.
*/

/*!
 * createImageDecoderInstance
 *
 * @param image_filepath: Image filepath.
 * @return: A pointer to an instance wrapper around the ImageDecoder class,
 * the memory should be freed by destroyImageDecoderInstance.
*/
ImageDecoderWrapper* createImageDecoderInstance
(
    const char* image_filepath,
    uint32_t* image_width,
    uint32_t* image_height,
    ImageColorType* image_color_type,
    uint8_t* image_bit_depth
);

/*!
 * destroyImageInstance
 *
 * @param image_decoder_instance: A pointer to the allocated ImageDecoderWrapper memory to be deallocated.
*/
void destroyImageDecoderInstance(ImageDecoderWrapper* image_decoder_wrapper);

/*!
 * getRawDataPtr
 *
 * @return: A pointer to a copy of the internal raw data,
 * caller is responsible for freeing the memory.
*/
uint8_t* getRawDataPtr(ImageDecoderWrapper* image_decoder_wrapper);

/*!
 * swapBytesOrder
 *
 * If bit depth > 8 bits, raw data will be reoder, meaning if it's LSB it'll become MSB,
 * and if it's MSB it'll become MSB.
 *
 * @return
*/
void swapBytesOrder(ImageDecoderWrapper* image_decoder_wrapper);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // IMAGE_DECODER_WRAPPER_H
