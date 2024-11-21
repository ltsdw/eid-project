# EID-Project

EID (Educational Image Decoder) focuses on teaching the ways of image decoding.
The project focus more on teaching than saving lines of code or being the most optmized possible, with easier syntaxes and algorithms, with lots of documentation explaining most complicated concepts.

# Installing as dependency

The library is better suited for being installed using cmake's [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html), you can use it standalone from your project, or install library system-wide and use cmake's [find_package](https://cmake.org/cmake/help/latest/command/find_package.html).

## Using it as a dependency from your CMakeLists.txt project

Simple CMakeLists.txt as example:
```cmake
cmake_minimum_required(VERSION 3.15)
project(hello_world LANGUAGES C CXX)
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
#include <filesystem>
#include <cstdint>
#include <iostream>
#include <cassert>

#include "image-decoder/image-decoder.hpp"

static void setBytesPerPixel(image_decoder::ImageColorType color_type, uint8_t* bytes_per_pixel)
{
    switch (color_type)
    {
        case image_decoder::ImageColorType::GRAYSCALE:
            *bytes_per_pixel = 1;
            break;
        case image_decoder::ImageColorType::GRAYSCALE_ALPHA:
            *bytes_per_pixel = 2;
            break;
        case image_decoder::ImageColorType::RGB:
            *bytes_per_pixel = 3;
            break;
        case image_decoder::ImageColorType::RGBA:
            *bytes_per_pixel = 4;
            break;
        default:
            assert(false and "Color type not supported.\n");
    };
}

int main(int argc, const char** argv)
{
    std::filesystem::path image_filepath {"input-images/rgb_8_bits.png"};

    image_decoder::ImageDecoder decoder(image_filepath);
    decoder.swapBytesOrder(); // Case your system doesn't use MSB (certainly it won't)
    uint32_t widht { decoder.getImageWidth() };
    uint32_t heigth { decoder.getImageHeight() };
    uint8_t bit_depth { decoder.getImageBitDepth() };
    image_decoder::ImageColorType color_type { decoder.getImageColorType() };
    uint8_t bytes_per_pixel { 0 };

    setBytesPerPixel(color_type, &bytes_per_pixel);

    utils::Bytes& my_image_raw_data =  decoder.getRawDataRef();

    /*!
     * Here you would do real work with the raw bytes of my_image_raw_data,
     * but I'm only priting info about the image for simplicity
    */
    std::cout << "file: " << image_filepath << "\n";
    std::cout << "width: " << widht << "\n";
    std::cout << "heigth: " << heigth << "\n";
    std::cout << "bit depth: " << (uint32_t)bit_depth << "\n";
    std::cout << "bytes per pixel: " << (uint32_t)bytes_per_pixel << "\n\n";

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

    ImageDecoderWrapper* image_decoder_wrapper = createImageDecoderInstance
    (
        "input-images/rgb_8_bits.png",
        &width,
        &height,
        &image_color_type,
        &image_bit_depth
    );

    uint8_t* my_image_raw_data = getRawDataPtr(image_decoder_wrapper);

    /*!
     * Here you would do real work with the raw bytes of my_image_raw_data,
     * but I'm only priting info about the image for simplicity.
    */

    printf("Image width: %d\n", width);
    printf("Image height: %d\n", height);
    printf("Image color type: %d\n", image_color_type);
    printf("Image bit depth: %d\n", image_bit_depth);

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
project(hello_world LANGUAGES C CXX)

find_package(eid_project REQUIRED)

add_executable(hello_world src/hello_world.cpp)
target_link_libraries(hello_world EID::eid_project)
```

# Todo-List
- [ ] Support < 8 bits depth for png images.
- [ ] Support interlacing method for png images.
- [ ] Support indexed images.
- [ ] Support decoding ppm images.
- [ ] Support decoding bmp images.
- [ ] Support decoding tiff images.
