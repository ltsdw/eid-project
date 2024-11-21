#include "utils/utils.hpp"

namespace utils
{

bool useNetworkByteOrder()
{
    /*!
     * I'm using bit_cast here just so clang-tidy doesn't complain
     * about how unsafe the C-style cast and reinterpret_cast are. But both works.
     *
     * We just care about the least significant byte, on LE this will be 0x0001
     * where the least significant byte will 01, on BE it will arrenged in memory as 0x0100
     * the least significant bit will 00.
     * (the number is the same for both LE and BE, the number 1, but how it's arrenged in memory that is differnt)
    */

    constexpr const static uint16_t test_byte = 0x1;
    const static bool has_msb = *std::bit_cast<const uint8_t*, const uint16_t*>(&test_byte) != 0x1;
    return has_msb;
}

uint64_t convertFromNetworkByteOrder(uint64_t value)
{
    /*!
     * I'm using bit_cast here just so clang-tidy doesn't complain
     * about how unsafe the C-style cast and reinterpret_cast are. But both works.
     *
     * We just care about the least significant byte, on LE this will be 0x0001
     * where the least significant byte will 01, on BE it will arrenged in memory as 0x0100
     * the least significant bit will 00.
     * (the number is the same for both LE and BE, the number 1, but how it's arrenged in memory that is differnt)
    */
    const static bool use_network_byte_order = useNetworkByteOrder();

    if (use_network_byte_order) { return value; }

    /*!
     * It will be easier to visualize the magic of byte swapping if checking the examples.
     * But it's really simple, nothing fancy, we shift the bytes we want to discard,
     * selectively isolate the byte we're interested applying a mask to it
     * and finally add the bytes again in the rearrenged order.
     *
     * for example:
     * value = 0x1122334455667788
     *
     * (value >> 56) = 0x0000000000000011
     *
     * (this will shift 56 bits to the right,
     * meaning 22334455667788 will be thrown away and the right of it will be padded with 0's)
     *
     * 0x0000000000000011 & 0x00000000000000FF = 0x0000000000000011
     *
     * (The mask above does nothing, but we add it just to make it consistent with the rest)
     * -----------------------------------------------------------------------------------------
     *
     * (value >> 40) = 0x0000000000112233
     *
     * (this will shift 40 bits to the right,
     * meaning 334455667788 will be thrown away and the right of it will be padded with 0's)
     *
     * 0x0000000000112233 & 0x000000000000FF00 = 0x0000000000002200
     *
     * (Here the mask will isolate of the byte '22' all the other bytes won't be present)
     *
     * 0x0000000000000011 | 0x0000000000002200 = 0x0000000000002211
     *
     * (And finally we just combine the both values together)
     *
     * We just repeat it until all is inverted.
    */
    return
    (
        (value >> 56) & (uint64_t)0xFF         |
        (value >> 40) & (uint64_t)(0xFF) << 8  |
        (value >> 24) & (uint64_t)(0xFF) << 16 |
        (value >> 8)  & (uint64_t)(0xFF) << 24 |
        (value << 8)  & (uint64_t)(0xFF) << 32 |
        (value << 24) & (uint64_t)(0xFF) << 40 |
        (value << 40) & (uint64_t)(0xFF) << 48 |
        (value << 56) & (uint64_t)(0xFF) << 56
    );
} // convertFromNetworkByteOrder

uint32_t convertFromNetworkByteOrder(uint32_t value)
{
    const static bool use_network_byte_order = useNetworkByteOrder();

    if (use_network_byte_order) { return value; }

    return
    (
        (value >> 24) & (uint32_t)(0xFF)       |
        (value >> 8)  & (uint32_t)(0xFF)  << 8 |
        (value << 8)  & (uint32_t)(0xFF)  << 16|
        (value << 24) & (uint32_t)(0xFF)  << 24
    );
} // convertFromNetworkByteOrder

uint16_t convertFromNetworkByteOrder(uint16_t value)
{
    const static bool use_network_byte_order = useNetworkByteOrder();

    if (use_network_byte_order) { return value; }

    return
    (
        (value >> 8) & (uint16_t)(0xFF)       |
        (value << 8) & (uint16_t)(0xFF) << 8
    );
} // convertFromNetworkByteOrder

uint32_t calculateCRC32(CBytes& data, uint32_t initial_value, uint32_t final_xor_value) noexcept
{
    static constexpr uint32_t POLYNOMIAL{0xEDB88320};
    uint32_t remainder = initial_value;

    for (auto byte : data)
    {
        remainder = remainder ^ (static_cast<uint32_t>(byte));

        for (uint8_t j = 0; j < 8; ++j)
        {
            if (remainder & 0x1)
            {
                /*!
                 * This is a naive implementation, it's easier to understand because
                 * it similar to the mathematical formula for calculating the crc,
                 * yet there's some ways to improve performance, one of which, is to use
                 * a lookup table with pre-calculated effects which each bit has on the crc
                 * this lookup table is the same, so you can either copy-paste it making it
                 * statically initialized, or you can dynamically initialize it, either way is
                 * better than this naive implementation, performance-wise speaking obviously.
                 * For our purposes here, this is fine.
                 *
                 * See: https://barrgroup.com/blog/crc-series-part-3-crc-implementation-code-cc
                 * (specifically in the "Code Cleanup" section)
                */
                remainder = (remainder >> 1) ^ POLYNOMIAL;
            } else
            {
                remainder = remainder >> 1;
            }
        }
    }

    return remainder ^ final_xor_value;
} // calculateCRC32

void appendNBytes(Bytes& dest, CBytes& src, Bytes::difference_type n_bytes)
{
    dest.reserve(dest.size() + (src.size() - n_bytes));
    dest.insert(dest.end(), src.begin(), src.begin() + n_bytes);
} // appendNBytes

bool matches(CBytes& lhs, const std::string& rhs) noexcept
{
    const static auto lambda = [](Byte b, uint8_t c) { return b == Byte{c}; };

    return lhs.size() == rhs.size() and
        std::equal
        (
            lhs.begin(),
            lhs.end(),
            rhs.begin(),
            lambda
        );
} // matches
} // namespace utils
