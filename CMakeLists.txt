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
    "${PROJECT_SOURCE_DIR}/src/image-decoder/image-decoder.cpp"
    "${PROJECT_SOURCE_DIR}/src/image-formats/png-format.cpp"
    "${PROJECT_SOURCE_DIR}/src/utils/utils.cpp"
    "${PROJECT_SOURCE_DIR}/src/utils/zlib-stream-manager.cpp"
)

# Alias for locally usage
add_library(EID::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib"
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    ZLIB::ZLIB
)

# Add compiler flags
include("${PROJECT_SOURCE_DIR}/cmake/compiler-flags.cmake")

# Install targets system-wide
include("${PROJECT_SOURCE_DIR}/cmake/eid-library-install.cmake")

# Image decoder wrapper setup and installation
include("${PROJECT_SOURCE_DIR}/cmake/eid-wrapper-install.cmake")

# Setup configure cmake file
include("${PROJECT_SOURCE_DIR}/cmake/config-setup.cmake")

# Build tests
option(BUILD_TESTS "Enable building tests" OFF)

if (DEBUG_ALLOCATOR)
    target_compile_definitions(
        ${PROJECT_NAME}
        PUBLIC
        DEBUG_ALLOCATOR=${DEBUG_ALLOCATOR}
    )
endif()

if (BUILD_TESTS)
    find_package(TIFF REQUIRED)
    file(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/tests/build")
    add_subdirectory("${PROJECT_SOURCE_DIR}/tests" "${PROJECT_SOURCE_DIR}/tests/build")
endif()
