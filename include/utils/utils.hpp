#pragma once

#include <cstring>
#include <string>
#include <stdexcept>
#include <zlib.h>

#include "utils/typings.hpp"

namespace utils
{
/*!
 * These functions will be useful for converting from network byte order (big-endian) to little endian.
 * Although we should technically use the hton* and ntoh* functions variations from #include <arpa/inet.h>,
 * it's cool to see how they could be implemented.
 *
 * https://en.wikipedia.org/wiki/Endianness
 *
 * More on converting network-byte-order to little endian here:
 * https://embetronicx.com/tutorials/p_language/c/little-endian-and-big-endian/
*/

/*!
 * useNetworkByteOrder
 *
 * @return: True if the host machine uses MSB (most significant byte).
*/
bool useNetworkByteOrder();

/*!
 * convertFromNetworkByteOrder
 *
 * @param value: Value.
 * @return: Converted value if the host machine uses LSB.
*/
uint64_t convertFromNetworkByteOrder(uint64_t value);
/*!
 * convertFromNetworkByteOrder
 *
 * @param value: Value.
 * @return: Converted value if the host machine uses LSB.
*/
uint32_t convertFromNetworkByteOrder(uint32_t value);
/*!
 * convertFromNetworkByteOrder
 *
 * @param value: Value.
 * @return: Converted value if the host machine uses LSB.
*/
uint16_t convertFromNetworkByteOrder(uint16_t value);

/*!
 * unsafe_cast
 *
 * This function is just so clang-tidy shut up a bit
 * about the dangers of reinterpret_cast.
 *
 * @param cast: value to be casted to.
 * @return: the casted value.
*/
template <typename To, typename From>
To unsafe_cast(const From& cast)
{
    return reinterpret_cast<To>(cast);
}

/*!
 * calculateCRC32
 *
 * Calculates the crc of data.
 *
 * @param data: Data to be calculated the crc.
 * @param initial_value: Initial value to start calculations useful for when there's a previous,
 *                       calculation we want to take into account.
 * @param final_xor_value: Value which the crc should be XORed after it's calculated.
 * @return: The crc calculated of data.
 *
 * CRC-32
 *
 * I'm no mathematician, but I'll try my best to explain this:
 *
 * To calculate the crc we will be doing a "long division" which in base 2
 * is a bit different than the usual base 10 division we're used to
 * (more on that in the links below).
 *
 * When learning about calculating crc manually, you will often see some terms of division, like:
 *
 * Divisor = this is where we plug or chosen polynomial (which isn't arbitrary, both the sender
 * and receiver must know it previously, for crc-32, we can use 0xEDB88320 or 0x04C11DB7,
 * with the first we get a reversed (or reflected) representation of the crc (right to left),
 * the other we process bits from left to right, as png store data in network-byte-order,
 * we'll be using the first one.
 *
 * Quotient = this we simply ignore, we won't use it for our crc calculation.
 *
 * Remainder = this is our crc, same as in a long division, we have remainder,
 * and here it's what we're looking for.
 *
 * A property interseting about the polynomial chosen, is that its MSB is always 1,
 * so taken for example a polynomial of 5 bits, x⁵ + x³ + x + 1, would be equivalent to:
 * 0b11001, supposing our system have a imaginary type uint4_t, which only holds 4 bits,
 * what should we do with the extra bit then 0b'1' 1001? Because it's always a 1, we can
 * make some assumptions about it, for example, when we XOR 1 with 1, it becomes 0, which
 * is useless for us when they appear on the right as we're reading from right to left,
 * as it would be if it was on the left when we're reading from left to right.
 *
 * All of that said, when we shift the bit 1 of our imaginary byte size 4, we are aligning it
 * with the bit 1 of our divisor (polynomial):
 *
 * 0b'1' 1001
 *     0b1010 << 1
 *--------------
 * 0b'1' 1001
 * 0b'1' 0100
 *
 * Again, this bit doesn't exist in our polynomial, because our imaginary byte only holds 4 bits,
 * we just assume it's there for mathematical purposes, when we shift our byte, in reality,
 * the bit simply drops into the oblivion, but it doesn't matter
 * because it would become 0 once we XORed it if it was really there.
 * Pretty neat, huh?
 *
 * Now that we have the bit 1 of our byte aligned with the byte 1 of our polynomial,
 * we can divide (do the XOR operation).
 *
 *  0b'1' 1001
 * ^
 *  0b'1' 0100
 * ------------
 *  0b'0' 1101
 *
 *  These last 4 bits is our crc, for our hypothetical 4 bits size byte.
 *  Of course that, if we had more bytes, we would continue dividing
 *  carryring the remainder (crc) to the next byte and XOR it with the byte
 *  until we finished all data.
 *  The operation above take into the account the LSB, which I feel most natural
 *  to show an example, but we're for png exactly, we're going to go the other way around,
 *  so just imagine the same operation, but with the bits flipped.
 *
 *
 *
 * Some simple steps just to start:
 *
 * 1- We initialize a register that we will put our calculated crc,
 * usually the values can be 0 for a more simple crc implementation,
 * or ~0x0U (which will set all bits) for more robust algorithms, like the crc-32.
 * Actually the algorithm is almost the same for all crc (8, 16, 32, 64)
 * but we spice things up a bit, to get better results on detecting errors
 * (crc doesn't repair data or where the message was corrupted,
 * it only tells that some data were corrupted).
 *
 * remainder = ~0x0U // initializing the crc with all bits set
 *
 * 2- We then loop over each byte, and for each byte we XOR it with the content
 * of the remainder (this way we take into account the effect the byte before had on the crc)
 *
 * for each byte in bytes:
 *     remainder = remainder ^ byte
 *
 * 3- We loop over each bit of the byte, while doing so we check if the LSB is 1, if it's
 * we divide it by our polynomial, if not we shift 1 bit to the right.
 * The step above is important so we align dividend (see the example above on why we need
 * to align both the dividend (our byte) with the divisor (the polynomial) before we can divide.
 *
 * for _ in range 0 to 8:
 *     if (remainder & 0x1):
 *         remainder = (remainder >> 1) ^ POLYNOMIAL
 *     else:
 *         remainder = remainder >> 1
 *
 * 4- Once all bytes were processed, we are left with a remainder (crc) with the effect of all bits,
 * in all bytes of our data, specific for crc-32 used by png, the only thing left is
 * to XOR this final remainder with ~0x0U again, and we're done and we can return it.
 *
 * return (remainder ^ ~0x0U)
 *
 *
 * Note:
 * If you're going to check how to calculate the crc usign the MSB instead, you're going to note
 * that we need to shift the bits of our byte so to make our 8 bits the first bits, we don't need
 * to do that for the LSB, because they already are in place, so for example,
 * if we're doing a crc-16, our byte only have 8 bits, so to start calculating we would need to shift
 * our byte 8 bits to the left:
 *
 * 0b00000000 01000111 << 8
 * -------------------
 * 0b01000111 00000000
 *
 * basically in code it would look like:
 *
 * remainder = remainder ^ (byte << 8)
 *
 *
 *
 * Some sources I read/watched to understand:
 *
 * https://www.w3.org/TR/png/#D-CRCAppendix
 * https://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks
 * https://www.youtube.com/watch?v=izG7qT0EpBw
 * https://barrgroup.com/downloads/code-crc-c
 * https://barrgroup.com/embedded-systems/how-to/additive-checksums
 * https://barrgroup.com/embedded-systems/how-to/crc-math-theory
 * https://barrgroup.com/embedded-systems/how-to/crc-calculation-c-code
*/
[[nodiscard]] uint32_t calculateCRC32(
    typings::CBytes& data,
    uint32_t initial_value = 0xFFFFFFFF,
    uint32_t final_xor_value = 0xFFFFFFFF
) noexcept;

/*!
 * appendNBytes
 *
 * Append length bytes from the source vector bytes to the destination vector bytes,
 * if offset is provided, it starts appending bytes from dest.begin() + offset of the destination vector.
 *
 * @param src: Source vector bytes.
 * @param dest: Destination vector bytes.
 * @param n_bytes: Number bytes from source that should be appened to destination vector.
 * @return
*/
void appendNBytes(typings::CBytes& src, typings::Bytes& dest, typings::Bytes::difference_type n_bytes);

/*!
 * matches
 *
 * Compares the vector bytes and the string to see if their bytes matches.
 *
 * @param lhs: Vector bytes to be matched against the string.
 * @param rhs: String to be matched agains the vector bytes.
 * @return: True if they match.
*/
bool matches(typings::CBytes& lhs, const std::string& rhs) noexcept;

/*!
 * readAndAdvanceIter
 *
 * @param begin: Begin iterator.
 * @param end: End iterator.
 * @return: The the value reinterpreted as T with the bytes between begin and end iterator.
 * @throw out_of_range exception in case the difference between begin and end
 * is less than the size necessary to create a type T
*/
template <typename T>
T readAndAdvanceIter(typings::Bytes::const_iterator& begin, typings::Bytes::const_iterator& end)
{
    T value;
    auto size_type = static_cast<typings::Bytes::difference_type>(sizeof(T));

    if (std::distance(begin, end) < size_type)
    {
        throw std::out_of_range("Not enough bytes to be read.\n");
    }

    std::memcpy(&value, &(*begin), size_type);

    begin += size_type;

    return value;
}

/*!
 * isWithinBoundaries
 *
 * @param begin_boundary: Container begin iterator.
 * @param end_boundary: Container end iterator.
 * @param begin_iterator: Begin iterator offset.
 * @param end_iterator: End iterator offset.
 * @return: True of if the iterators fall within the boundaries.
*/
template <typename Iterator>
typename std::enable_if
<
    std::is_base_of
    <
        std::input_iterator_tag, typename std::iterator_traits<Iterator>::iterator_category
    >::value,
    bool
>::type isWithinBoundaries(Iterator begin_boundary, Iterator end_boundary, Iterator begin_iterator, Iterator end_iterator)
{
    return (begin_iterator >= begin_boundary and begin_iterator <= end_boundary)
           and (end_iterator >= begin_boundary and end_iterator <= end_boundary);
}

/*!
 * isWithinBoundaries
 *
 * @param begin_boundary: Container begin iterator.
 * @param end_boundary: Container end iterator.
 * @param iterator: Iterator offset.
 * @return: True of if the iterator fall within the boundaries.
*/
template <typename Iterator>
typename std::enable_if
<
    std::is_base_of
    <
        std::input_iterator_tag, typename std::iterator_traits<Iterator>::iterator_category
    >::value,
    bool
>::type isWithinBoundaries(Iterator begin_boundary, Iterator end_boundary, Iterator iterator)
{
    return (iterator >= begin_boundary and iterator <= end_boundary);
}
} // namespace utils
