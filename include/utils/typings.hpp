#pragma once

#include <vector>

#ifdef DEBUG_ALLOCATOR
#include "utils/debugging.hpp"
#endif

/*!
 * Some forward declarations for includes for typing,
 * sometimes including a header also includes a bunch of functions
 * and classes which doesn't even are in the header included.
*/

// include/image-formats/png-format.hpp
namespace image_formats::png_format
{
    class PNGFormat;
} // namespace image_formats::png_format

namespace utils::typings
{
/*!
 * Some structs and type aliases
*/


using Byte = std::byte;

/*!
 * We could use std::array for most part of the program where
 * a structure has a fixed size, as signature, chunk types, etc.
 * But for any other dynamic chain of bytes handling and passing std::array
 * is really not viable, so we use std::vector instead.
*/
#ifdef DEBUG_ALLOCATOR
using BytesAllocator = debugging::DebugAllocator<Byte>;
using Bytes = std::vector<Byte, BytesAllocator>;
using CBytes = const Bytes;
#else
using Bytes = std::vector<Byte>;
using CBytes = const Bytes;
#endif // LOGGING_ALLOCATOR

/*!
 * This is enum is needed for the wrapper,
 * any changes here must be reflected in image-decoder-wrapper.h
*/
enum ImageColorType
{
    INVALID_COLOR_TYPE = -1,
    GRAYSCALE_COLOR_TYPE,
    RGB_COLOR_TYPE,
    INDEXED_COLOR_TYPE,
    GRAYSCALE_AND_ALPHA_COLOR_TYPE,
    RGBA_COLOR_TYPE,
}; // enum ImageColorType

/*!
 * Some types and type aliases for easy of documentation.
*/

// TODO: Create string representation of the enums.
enum class ImageFormat
{
    PNG_FORMAT_TYPE = 0x1,
}; // enum class ImageFormat

using image_formats::png_format::PNGFormat;
} // utils::typings
