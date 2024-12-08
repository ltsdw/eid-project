#pragma once

#include <zlib.h>

#include "utils/typings.hpp"

namespace utils
{
class ZlibStreamManager
{
public:

    /*!
     * ZlibStreamManager
     *
     * @param output_chunk_size: Specify how many bytes will be outputed to the output buffer at time.
    */
    ZlibStreamManager(uint32_t buffer_size = 4096);
    ~ZlibStreamManager();
    ZlibStreamManager(ZlibStreamManager&&);
    ZlibStreamManager& operator=(ZlibStreamManager&&);
    ZlibStreamManager(const ZlibStreamManager&) = delete;
    ZlibStreamManager& operator=(const ZlibStreamManager&) = delete;

    /*!
     * decompressData
     *
     * @param compressed_data: Zlib compressed data bytes vector.
     * @param decompressed_data: Output vector for the decompressed data bytes.
     * @return
    */
    void decompressData
    (
        typings::CBytes& compressed_data,
        typings::Bytes& decompressed_data
    );

private:
    /*!
     * growBuffer
     *
     * Resizes the internal buffer to hold new_capacity_size elements,
     * new_capacity_size must be greater than the current buffer size.
     *
     * @param new_capacity_size: New capacity size of the internal buffer.
     *
     * @return
    */
    void growBuffer(uint32_t new_capacity_size);

private:
    z_stream m_z_stream
    {
        .next_in = Z_NULL,
        .avail_in = Z_NULL,
        .next_out = Z_NULL,
        .avail_out = Z_NULL,
        .zalloc = Z_NULL,
        .zfree = Z_NULL,
        .opaque = Z_NULL
    };
    utils::typings::Bytes m_buffer;
}; // class ZlibStreamManager
} // namespace utils
