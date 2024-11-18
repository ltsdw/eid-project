#pragma once

#include <zlib.h>

#include "utils.hpp"

namespace utils {
    class ZlibStreamManager
    {
        public:
            ZlibStreamManager();
            ~ZlibStreamManager();
            ZlibStreamManager(const ZlibStreamManager&) = delete;
            ZlibStreamManager(ZlibStreamManager&&) = delete;
            ZlibStreamManager& operator=(const ZlibStreamManager&) = delete;
            ZlibStreamManager& operator=(ZlibStreamManager&&) = delete;

            /*!
             * decompressData
             *
             * @param compressed_data: Zlib compressed data bytes vector.
             * @param decompressed_data: Output vector for the decompressed data bytes.
             * @param output_chunk_size: Specify how many bytes will be outputed to the output buffer at time.
             * @return: TODO
            */
            void decompressData(CBytes& compressed_data, Bytes& decompressed_data, uint32_t output_chunk_size = 4096);

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
    }; // class ZlibStreamManager
} // namespace utils
