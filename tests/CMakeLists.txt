add_executable(
    run_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/run-tests.cpp"
    "${CMAKE_SOURCE_DIR}/src/utils/utils.cpp"
    "${CMAKE_SOURCE_DIR}/src/utils/zlib-stream-manager.cpp"
    "${CMAKE_SOURCE_DIR}/src/image-decoder/image-decoder.cpp"
    "${CMAKE_SOURCE_DIR}/src/image-formats/png-format.cpp"
)

target_include_directories(
    run_tests
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/include"
)

target_compile_features(run_tests PRIVATE cxx_std_20)

target_link_libraries(
    run_tests
    PRIVATE
    ZLIB::ZLIB
    TIFF::TIFF
)
