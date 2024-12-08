# EID-Project

EID (Educational Image Decoder) focuses on teaching the ways of image decoding.
The project focus more on teaching than saving lines of code or being the most optmized possible, with easier syntaxes and algorithms, with lots of documentation explaining most complicated concepts.

# Where to start?

First, most information about a specific topic is within the headers, although the headers explains a lot of topics, code has also important information.

The entry point is the ImageDecoder class constructor, from there, based on the extension of the file, a specific member function for that image format will be called to start the decoding process.
So taking for example a PNG file, the process goes like:

1. A filepath is fed to the ImageDecoder constructor.
2. ImageDecoder decides which function to call to load the file, for PNG files that will be ImageDecoder::loadPNGImage.
3. ImageDecoder::loadPNGImage will create a PNGFormat.
4. The PNGFormat object will be responsible for decompressing image data and passing this decompressed data to Scanlines object.
5. Finally the Scanlines object will process each scanline and apply the correct defilter to each of them, leaving a raw data structure representing the image.

The proccess above are just for the PNG format, different format has different steps before we can get the raw data, some format are trickier than others.

"Not all image formats are created equal" - Someone, somewhere

# Installing as dependency

The library is better suited for being installed using cmake's [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html), you can use it standalone from your project, or install the library system-wide and use cmake's [find_package](https://cmake.org/cmake/help/latest/command/find_package.html).

## Using it as a dependency from your CMakeLists.txt project

Simple CMakeLists.txt as example:
```cmake
cmake_minimum_required(VERSION 3.15)
project(hello_world LANGUAGES CXX)
include(FetchContent)

FetchContent_Declare(
    eid_project
    GIT_REPOSITORY "https://github.com/ltsdw/eid-project.git"
    GIT_TAG main
)

FetchContent_MakeAvailable(eid_project)

add_executable(hello_world src/hello_world.cpp)
target_link_libraries(hellow_world EID::eid_project)
```
Content's of the src/hello_world.cpp:

```cpp
#include <cstdlib>
#include <cassert>
#include <iostream>

#include "image-decoder/image-decoder.hpp"
#include "utils/typings.hpp"

int main(int argc, const char** argv)
{
    std::filesystem::path image_filepath {"input-images/rgba_16_bits.png"};

    image_decoder::ImageDecoder decoder(image_filepath);

    /*!
     * If you're using a png file, and need its raw 16 bit to be in LSB
     * for other programs, you'll need to swap its bytes from MSB to LSB.
    */
    decoder.swapBytesOrder();

    uint32_t width { decoder.getImageWidth() };
    uint32_t heigth { decoder.getImageHeight() };
    uint8_t bit_depth { decoder.getImageBitDepth() };
    utils::typings::ImageColorType color_type { decoder.getImageColorType() };
    uint32_t scanline_size { decoder.getImageScanlineSize() };
    uint32_t scanlines_size { decoder.getImageScanlinesSize() };
    uint8_t number_of_channels { decoder.getImageNumberOfChannels() };

    utils::typings::Bytes raw_data { decoder.getRawDataRGBA() };

    /*!
     * Only needed if the image were converted from one color type to another
     * and only if the internal cache isn't needed anymore.
     *
     * Calling it when there's no cache to clean will simply be no op.
    */
    decoder.resetCachedData();

    /*!
     * Here you would do real work with the raw bytes of my_image_raw_data,
     * but I'm only priting info about the image for simplicity
    */
    std::cout << "file: " << image_filepath << "\n";
    std::cout << "width: " << width << "\n";
    std::cout << "heigth: " << heigth << "\n";
    std::cout << "bit depth: " << (uint32_t)bit_depth << "\n";
    std::cout << "scanline size: " << scanline_size << "\n";
    std::cout << "scanlines size: " << scanlines_size << "\n";
    std::cout << "number of channels: " << (uint32_t)number_of_channels << "\n\n";
    std::cout << "color type: " << (int)(color_type) << "\n";

    return EXIT_SUCCESS;
}
```

# Wrapper for usage within C code
There's also a cpp wrapper, that provides an easy to use interface for plain C code.

First, you need to link your targets against **EID::eid_projectWrapper**, so your CMakeLists.txt should look like:

```cmake
cmake_minimum_required(VERSION 3.15)
project(hello_world_wrapper LANGUAGES C CXX)
include(FetchContent)

FetchContent_Declare(
    eid_project
    GIT_REPOSITORY "https://github.com/ltsdw/eid-project.git"
    GIT_TAG main
)

FetchContent_MakeAvailable(eid_project)

add_executable(hello_world_wrapper src/hello_world_wrapper.c)
target_link_libraries(hello_world_wrapper EID::eid_projectWrapper)
```

And for a simple usage example, the content's of src/hello_world_wrapper.c:

```c
#include <stdio.h>
#include <stdlib.h>

#include "image-decoder-wrapper/image-decoder-wrapper.h"

int main(int argc, const char** argv)
{
    uint32_t width = 0;
    uint32_t height = 0;
    ImageColorType image_color_type;
    uint8_t image_bit_depth = 0;
    uint8_t image_number_of_channels = 0;
    uint32_t image_scanline_size = 0;
    uint32_t image_scanlines_size = 0;
    uint32_t image_rgb_scanline_size = 0;
    uint32_t image_rgb_scanlines_size = 0;
    uint32_t image_rgba_scanline_size = 0;
    uint32_t image_rgba_scanlines_size = 0;

    const char* error = NULL;

    ImageDecoderWrapper* image_decoder_wrapper =
    createImageDecoderInstance
    (
        "input-images/rgba_16_bits.png",
        &width,
        &height,
        &image_color_type,
        &image_bit_depth,
        &image_number_of_channels,
        &image_scanline_size,
        &image_scanlines_size,
        NULL,
        NULL,
        NULL,
        NULL,
        &error
    );

    if (! image_decoder_wrapper)
    {
        printf("createImageDecoderInstance failed: %s\n", error);

        return EXIT_FAILURE;
    }

    /*!
     * If you're using a png file, if the image is 16 bit depth and you need its raw data to be in LSB
     * for usage in other programs, you'll need to swap its bytes from MSB to LSB.
     * This only affects 8 > bits png image.
    */
    int ret = swapBytesOrder(image_decoder_wrapper, &error);

    if (ret != 0)
    {
        printf("swapBytesOrder failed: %s\n", error);

        return EXIT_FAILURE;
    }

    uint8_t* raw_data = getRawDataBuffer(image_decoder_wrapper, &error);

    /*!
     * Usefull to convert between color types
    */
    uint8_t* raw_data_rgb = getRawDataRGBBuffer(image_decoder_wrapper, &error);

    if (! raw_data)
    {
        printf("getRawDataBuffer failed: %s\n", error);

        return EXIT_FAILURE;
    }

    if (! raw_data_rgb)
    {
        printf("getRawDataRGBBuffer failed: %s\n", error);

        return EXIT_FAILURE;
    }

    printf("Image width: %d\n", width);
    printf("Image height: %d\n", height);
    printf("Image color type: %d\n", image_color_type);
    printf("Image bit depth: %d\n", image_bit_depth);
    printf("Image number of channels: %d\n", image_number_of_channels);
    printf("Image scanline size: %d\n", image_scanline_size);
    printf("Image scanlines size: %d\n", image_scanlines_size);

    /*!
     * Only needed if the image were converted from one color type to another
     * and only if the internal cache isn't needed anymore.
     *
     * Calling it when there's no cache to clean will simply be no op.
    */
    resetCachedData(image_decoder_wrapper, &error);

    /*!
     * Remember to free all the allocated resources
    */
    freeRawDataBuffer(raw_data);
    freeRawDataBuffer(raw_data_rgb);
    destroyImageDecoderInstance(image_decoder_wrapper);

    return EXIT_SUCCESS;
}
```

## Standalone building
```
git clone https://github.com/ltsdw/eid-project
cd eid-project
cmake -S . -B build
cmake --build build
```

## Optionally installing system-wide
```
cmake --install build
```

## Using system-wide installed library

```cmake
cmake_minimum_required(VERSION 3.15)
project(hello_world LANGUAGES CXX)

find_package(eid_project REQUIRED)

add_executable(hello_world src/hello_world.cpp)
target_link_libraries(hello_world EID::eid_project)
```

# Todo-List
- [ ] Support interlacing method for png images.
- [ ] Support decoding ppm images.
- [ ] Support decoding bmp images.
- [ ] Support decoding tiff images.
