#include <stdexcept>

#include "utils/utils.hpp"
#include "utils/zlib-stream-manager.hpp"

namespace utils {

ZlibStreamManager::ZlibStreamManager(uint32_t buffer_size)
{
    int ret = inflateInit(&m_z_stream);

    if (ret != Z_OK)
    {
        throw std::runtime_error("Failed to initialize zlib stream.\n");
    }

    m_buffer.resize(buffer_size);
}

ZlibStreamManager::~ZlibStreamManager()
{
    inflateEnd(&m_z_stream);
}

ZlibStreamManager::ZlibStreamManager(ZlibStreamManager&&) = default;
ZlibStreamManager& ZlibStreamManager::operator=(ZlibStreamManager&&) = default;

void ZlibStreamManager::decompressData
(
    typings::CBytes& compressed_data,
    typings::Bytes& decompressed_data
)
{
    m_z_stream.next_in = std::bit_cast<Bytef*>(compressed_data.data());
    m_z_stream.avail_in = compressed_data.size();

    while (m_z_stream.avail_in > 0) // there's no more data to be processed when avail_in is 0
    {
        m_z_stream.next_out = std::bit_cast<Bytef*>(m_buffer.data());
        m_z_stream.avail_out = m_buffer.size();
        int ret = inflate(&m_z_stream, Z_NO_FLUSH);
        auto number_of_bytes_written = static_cast<typings::Bytes::difference_type>(m_buffer.size() - m_z_stream.avail_out );

        if (ret != Z_OK and ret != Z_STREAM_END)
        {
            throw std::runtime_error("Inflate error: " + std::to_string(ret) + "\n" + m_z_stream.msg + "\n");
        }

        appendNBytes(m_buffer, decompressed_data, number_of_bytes_written);
    }
}

} //namespace utils
