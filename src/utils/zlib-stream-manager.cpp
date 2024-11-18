#include "utils/zlib-stream-manager.hpp"

namespace utils {

ZlibStreamManager::ZlibStreamManager()
{
    int ret = inflateInit(&m_z_stream);

    if (ret != Z_OK)
    {
        throw std::runtime_error("Failed to initialize zlib stream.\n");
    }
}

ZlibStreamManager::~ZlibStreamManager()
{
    inflateEnd(&m_z_stream);
}

void ZlibStreamManager::decompressData(CBytes& compressed_data, Bytes& decompressed_data, uint32_t output_chunk_size)
{
    Bytes buffer(output_chunk_size);
    m_z_stream.next_in = std::bit_cast<Bytef*>(compressed_data.data());
    m_z_stream.avail_in = compressed_data.size();

    while (m_z_stream.avail_in > 0) // there's no more data to be processed when avail_in is 0
    {
        m_z_stream.next_out = std::bit_cast<Bytef*>(buffer.data());
        m_z_stream.avail_out = buffer.size();
        int ret = inflate(&m_z_stream, Z_NO_FLUSH);
        auto number_of_bytes_written = static_cast<Bytes::difference_type>( output_chunk_size - m_z_stream.avail_out );

        if (ret != Z_OK and ret != Z_STREAM_END)
        {
            throw std::runtime_error("Inflate error: " + std::to_string(ret) + "\n" + m_z_stream.msg + "\n");
        }

        appendNBytes(decompressed_data, buffer, number_of_bytes_written);
    }
}

} //namespace utils
