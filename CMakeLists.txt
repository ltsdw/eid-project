cmake_minimum_required(VERSION 3.14...3.30)

project(
    eid_project
    VERSION 0.1.0
    DESCRIPTION "Educational Image Decoder"
    LANGUAGES C CXX
)

find_package(ZLIB REQUIRED)

# The EID CPP Library
add_library(
    ${PROJECT_NAME}
    STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/image-decoder/image-decoder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/image-formats/png-format.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/utils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/zlib-stream-manager.cpp"
)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    ZLIB::ZLIB
)

# Install targets system-wide
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/install-instructions.cmake")

# Image decoder wrapper setup and installation
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/image-decoder-wrapper.cmake")

# Setup configure cmake file
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/config-setup.cmake")

# Build tests
option(BUILD_TESTS "Enable building tests" OFF)

if (BUILD_TESTS)
    find_package(TIFF REQUIRED)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests/build")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tests" "${CMAKE_CURRENT_SOURCE_DIR}/tests/build")
endif()
