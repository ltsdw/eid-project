#ifndef IMAGE_DECODER_WRAPPER_H
#define IMAGE_DECODER_WRAPPER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/*!
 * Some simple macros just for auto documentation on the return codes.
*/
#define SUCCESS 0
#define INVALID_ARGUMENTS -1
#define EXCEPTION -2

typedef enum
{
    INVALID_COLOR_TYPE = -1,
    GRAYSCALE_COLOR_TYPE,
    RGB_COLOR_TYPE,
    INDEXED_COLOR_TYPE,
    GRAYSCALE_AND_ALPHA_COLOR_TYPE,
    RGBA_COLOR_TYPE,
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
 * @param image_width: Optional pointer to store image's width.
 * @param image_height: Optional pointer to store image's height.
 * @param image_color_type: Optional pointer to store image's color type.
 * @param image_bit_depth: Optional pointer to store image's bit depth.
 * @param image_number_of_channels: Optional pointer to store image's number of channels.
 * @param image_scanline_size: Optional pointer to store image's scanline size.
 * @param image_scanlines_size: Optional pointer to store image's scanlines size.
 * @param image_rgb_scanline_size: Optional pointer to store image's rgb scanline size.
 * @param image_rgb_scanlines_size: Optional pointer to store image's rgb scanlines size.
 * @param image_rgba_scanline_size: Optional pointer to store image's rgba scanline size.
 * @param image_rgba_scanlines_size: Optional pointer to store image's rgba scanlines size.
 * @param error: If there's any error its message will be placed into it.
 * @return: A pointer to an instance wrapper around the ImageDecoder class,
 * the memory should be deallocated by destroyImageDecoderInstance.
 * NULL pointer will be returned in case of error.
 * The caller must check the 'error' parameter message
 * to see what happened in case of null pointer return.
*/
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
);

/*!
 * destroyImageInstance
 *
 * @param image_decoder_wrapper: Pointer to an instance of the ImageDecoder object to be deallocated.
 * @return
*/
void destroyImageDecoderInstance(ImageDecoderWrapper* image_decoder_wrapper);

/*!
 * getRawDataBuffer
 *
 * This function will allocate memory and copy the content of the internal raw data buffer to the new allocated buffer.
 * The caller must deallocate memory using freeRawDataBuffer.
 *
 * @param image_decoder_wrapper: Pointer to an instance of the ImageDecoder object.
 * @param error: If there's any error its message will be placed into it.
 * @return: A pointer to the allocated memory and copied content of the internal raw data buffer.
 * It's the caller responsability to deallocate this memory with freeRawDataBuffer.
 * NULL pointer will be returned in case of error.
 * The caller must check the 'error' parameter message
 * to see what happened in case of null pointer return.
*/
uint8_t* getRawDataBuffer(ImageDecoderWrapper* image_decoder_wrapper, const char** error);

/*!
 * getRawDataRGBBuffer
 *
 * This function will allocate memory and copy the content of the internal raw data buffer in
 * (red, green, blue, alpha) format to the new allocated buffer.
 *
 * The caller must deallocate memory using freeRawDataBuffer.
 *
 * @param image_decoder_wrapper: Pointer to an instance of the ImageDecoder object.
 * @param error: If there's any error its message will be placed into it.
 * @return: A pointer to the allocated memory and copied content of the internal raw data buffer.
 * It's the caller responsability to deallocate this memory with freeRawDataBuffer.
 * NULL pointer will be returned in case of error.
 * The caller must check the 'error' parameter message
 * to see what happened in case of null pointer return.
*/
uint8_t* getRawDataRGBBuffer(ImageDecoderWrapper* image_decoder_wrapper, const char** error);

/*!
 * getRawDataRGBABuffer
 *
 * This function will allocate memory and copy the content of the internal raw data buffer in
 * (red, green, blue, alpha) format to the new allocated buffer.
 *
 * The caller must deallocate memory using freeRawDataBuffer.
 *
 * @param image_decoder_wrapper: Pointer to an instance of the ImageDecoder object.
 * @param error: If there's any error its message will be placed into it.
 * @return: A pointer to the allocated memory and copied content of the internal raw data buffer.
 * It's the caller responsability to deallocate this memory with freeRawDataBuffer.
 * NULL pointer will be returned in case of error.
 * The caller must check the 'error' parameter message
 * to see what happened in case of null pointer return.
*/
uint8_t* getRawDataRGBABuffer(ImageDecoderWrapper* image_decoder_wrapper, const char** error);

/*!
 * freeRawDataBuffer
 *
 * Deallocates the memory allocated for buffer allocated by the getRawDataBuffer.
 *
 * @return
*/
void freeRawDataBuffer(uint8_t* buffer);

/*!
 * swapBytesOrder
 *
 * If bit depth > 8 bits, raw data will be reoder, meaning if it's LSB it'll become MSB,
 * and if it's MSB it'll become MSB.
 *
 * @param image_decoder_wrapper: Pointer to an instance of the ImageDecoder object.
 * @param error: If there's any error its message will be placed into it.
 * @return: On success this function will return 0, it will return -1 if the arguments are invalid or -2 if an exception happens.
 * The caller must check the 'error' parameter to see what happened in case of non-zero return.
*/
int swapBytesOrder(ImageDecoderWrapper* image_decoder_wrapper, const char** error);

/*!
 * resetCachedData
 *
 * Empties all cached data freeing its resources back to the system.
 * Original data won't be touched, leaving it in a usable state.
 *
 * @param image_decoder_wrapper: Pointer to an instance of the ImageDecoder object.
 * @param error: If there's any error its message will be placed into it.
 *
 * @return
*/
void resetCachedData(ImageDecoderWrapper* image_decoder_wrapper, const char** error);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // IMAGE_DECODER_WRAPPER_H
