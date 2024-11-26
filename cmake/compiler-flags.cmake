# Checks if the compiler supports the flags
include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("-Wnrvo" HAS_NRVO_FLAG)

# Add it globally if supported
if(HAS_NRVO_FLAG)
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wnrvo>")
endif()
