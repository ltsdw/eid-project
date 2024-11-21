# The EID CPP library wrapper for C usage

add_library(
    ${PROJECT_NAME}Wrapper
    STATIC
    "${CMAKE_SOURCE_DIR}/src/image-decoder-wrapper/image-decoder-wrapper.cpp"
)

target_include_directories(
    ${PROJECT_NAME}Wrapper
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/image-decoder-wrapper>
    $<INSTALL_INTERFACE:include/image-decoder-wrapper>
)

set_target_properties(
    ${PROJECT_NAME}Wrapper
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

target_link_libraries(
    ${PROJECT_NAME}Wrapper
    PRIVATE
    ${PROJECT_NAME}
)

# Install targets to its specific system-wide location
include(GNUInstallDirs)

install(
    TARGETS ${PROJECT_NAME}Wrapper
    EXPORT ${PROJECT_NAME}WrapperTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    EXPORT ${PROJECT_NAME}WrapperTargets
    FILE ${PROJECT_NAME}WrapperTargets.cmake
    NAMESPACE EID::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

# Export the target for locally usage without installing system-wide
export(
    TARGETS ${PROJECT_NAME}Wrapper
    FILE "${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}WrapperTargets.cmake"
    NAMESPACE EID::
)

