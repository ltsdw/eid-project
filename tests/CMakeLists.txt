# Build the image tests

add_executable(
    run_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/src/run-tests/utils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/run-tests/run-tests.cpp"
)

target_include_directories(
    run_tests
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

set_target_properties(
    run_tests
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/run-tests"
)

target_compile_features(
    run_tests
    PRIVATE
    cxx_std_20
)

target_link_libraries(
    run_tests
    PRIVATE
    TIFF::TIFF
    EID::${PROJECT_NAME}
)

# Build the image decoder wrapper tests
add_executable(
    image_decoder_wrapper_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/src/image-decoder-wrapper-tests/image-decoder-wrapper-tests.c"
)

set_target_properties(
    image_decoder_wrapper_tests
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/image-decoder-wrapper-tests"
)

target_link_libraries(
    image_decoder_wrapper_tests
    PRIVATE
    EID::${PROJECT_NAME}Wrapper
)
