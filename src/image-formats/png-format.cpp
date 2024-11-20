#include <iostream>
#include <string>
#include <cmath>

#include "image-formats/png-format.hpp"
#include "utils/utils.hpp"
#include "utils/zlib-stream-manager.hpp"

namespace image_formats::png_format
{

PNGFormat::PNGFormat(const std::filesystem::path& image_filepath)
{
    m_image_stream.exceptions(std::fstream::badbit | std::fstream::failbit);
    m_image_stream.open(image_filepath, std::fstream::binary);

    if (not m_image_stream.is_open())
    {
        std::cerr << "File isn't open, exiting.\n";
        std::exit(EXIT_FAILURE);
    }

    utils::ZlibStreamManager z_lib_stream_manager{};
    utils::Bytes decompressed_data;
    readNBytes(m_signature, SIGNATURE_FIELD_BYTES_SIZE);

    // Parses all chunks
    while (true)
    {
        Chunk chunk;

        if (not readNextChunk(chunk)) { break; }

        if (utils::matches(chunk.m_chunk_type, "IHDR"))
        {
            fillIHDRData(chunk.m_chunk_data);
        } else if (utils::matches(chunk.m_chunk_type, "IDAT"))
        {
            z_lib_stream_manager.decompressData(chunk.m_chunk_data, decompressed_data);
        }
    }

    // Start scanlines structure
    Scanlines m_scanlines(
        m_ihdr.width,
        m_ihdr.height,
        m_ihdr.bit_depth,
        m_ihdr.color_type
    );

    // Defilter the scanlines
    m_scanlines.defilterData(decompressed_data, m_defiltered_data);
} // PNGFormat::PNGFormat

PNGFormat::~PNGFormat()
{
    m_image_stream.close();
} // PNGFormat::~PNGFormat

void PNGFormat::readNBytes(utils::Bytes& data, std::streamsize n_bytes)
{
    m_image_stream.read(
        std::bit_cast<char*, utils::Byte*>(data.data()),
        n_bytes
    );
} // PNGFormat::readNBytes

void PNGFormat::readNBytes(void* data, std::streamsize n_bytes)
{
    m_image_stream.read(
        std::bit_cast<char*, void*>(data),
        n_bytes
    );
} // PNGFormat::readNBytes

bool PNGFormat::readNextChunk(Chunk& chunk)
{
    uint32_t length{0};
    uint32_t crc{0};
    uint32_t data_crc{0};

    readNBytes(&length, CHUNK_LENGTH_FIELD_BYTES_SIZE);
    readNBytes(chunk.m_chunk_type, CHUNK_TYPE_FIELD_BYTES_SIZE);

    length = utils::convertFromNetworkByteOrder(length);

    if (utils::matches(chunk.m_chunk_type, "IEND")) { return false; }

    chunk.m_chunk_data.resize(length);
    readNBytes(chunk.m_chunk_data, length);
    readNBytes(&crc, CRC_FIELD_BYTES_SIZE);

    crc = utils::convertFromNetworkByteOrder(crc);

    /*!
     * We first calculate the crc of the first 4 bytes (the chunk type)
     * then we use the crc of these 4 bytes to calculate the rest of the bytes,
     * it would be possible to calculate this only once if we appended both vectors togheter
     * but that would be a waste of space.
     *
     * The final_xor_value is 0 at first because we only final XOR the crc against 0xFFFFFFFF
     * when all the bytes' crc is already calculated.
    */
    data_crc = utils::calculateCRC32(chunk.m_chunk_type, 0xFFFFFFFF, 0);
    data_crc = utils::calculateCRC32(chunk.m_chunk_data, data_crc);

    std::string string(std::bit_cast<char*>(chunk.m_chunk_type.data()), CHUNK_TYPE_FIELD_BYTES_SIZE);

    if (data_crc != crc)
    {
        throw std::runtime_error("Crc doesn't match, data may be corrupted data.\n");
    }

    return true;
} // PNGFormat::readNextChunk

void PNGFormat::fillIHDRData(utils::CBytes& data)
{
    if (not (data.size() == IHDR_CHUNK_BYTES_SIZE))
    {
        throw std::runtime_error("IHDR chunk mismatch size.\n");
    }

    auto begin = data.begin();
    auto end = data.end();

    m_ihdr.width = utils::convertFromNetworkByteOrder(utils::readAndAdvanceIter<uint32_t>(begin, end));
    m_ihdr.height = utils::convertFromNetworkByteOrder(utils::readAndAdvanceIter<uint32_t>(begin, end));
    m_ihdr.bit_depth = utils::readAndAdvanceIter<uint8_t>(begin, end);
    m_ihdr.color_type = utils::readAndAdvanceIter<uint8_t>(begin, end);
    m_ihdr.compression_method = utils::readAndAdvanceIter<uint8_t>(begin, end);
    m_ihdr.filter_method = utils::readAndAdvanceIter<uint8_t>(begin, end);
    m_ihdr.interlaced_method = utils::readAndAdvanceIter<uint8_t>(begin, end);
} // PNGFormat::fillIHDRData

size_t PNGFormat::getScanlinesSize() const noexcept
{
    return m_defiltered_data.size();
} // PNGFormat::getScanlinesSize

utils::Bytes& PNGFormat::getRawDataRef() noexcept
{
    return m_defiltered_data;
} // PNGFormat::getRawDataRef

utils::Bytes PNGFormat::getRawDataCopy() noexcept
{
    return m_defiltered_data;
} // PNGFormat::getRawDataCopy

uint8_t* PNGFormat::getRawDataPtr()
{
    return std::bit_cast<uint8_t*>(m_defiltered_data.data());
} // PNGFormat::getRawDataPtr

uint32_t PNGFormat::getImageWidth() const noexcept
{
    return m_ihdr.width;
} // PNGFormat::getImageWidth

uint32_t PNGFormat::getImageHeight() const noexcept
{
    return m_ihdr.height;
} // PNGFormat::getImageHeight

uint8_t PNGFormat::getImageBitDepth() const noexcept
{
    return m_ihdr.bit_depth;
} // PNGFormat::getImageBitDepth

uint8_t PNGFormat::getImageColorType() const noexcept
{
    return m_ihdr.color_type;
} // PNGFormat::getImageColorType

void PNGFormat::swapBytesOrder()
{
    if (m_ihdr.bit_depth <= 8) { return; }

    for (auto it = m_defiltered_data.begin(); it != m_defiltered_data.end(); it += 2)
    {
        std::swap(*it, *(it+1));
    }
}

Scanlines::Scanlines
(
    uint32_t width,
    uint32_t height,
    uint8_t bit_depth,
    uint8_t color_type
)
{
    // TODO: Add support for bits per pixel 1, 2, 4 bits.
    if (bit_depth < 0x8 or bit_depth > 0x10) { throw std::runtime_error("Bit depth not supported.\n"); }

    uint8_t number_of_channels =
        (color_type == 0x0) ? 1 :
        (color_type == 0x2) ? 3 :
        //TODO: Add support indexed color
        (color_type == 0x4) ? 2 :
        (color_type == 0x6) ? 4 : throw std::runtime_error("Color type not supported.\n");
    m_bytes_per_pixel = number_of_channels * (bit_depth / 8);
    m_scanline_size = static_cast<utils::Bytes::difference_type>(width) * m_bytes_per_pixel;
    m_scanlines_size = m_scanline_size * height;
} // Scalines::Scalines

void Scanlines::defilterData(utils::CBytes& filtered_data, utils::Bytes& defiltered_data)
{
    // Initialize and resize all the space needed to accommodate all the scanlines
    defiltered_data.resize(m_scanlines_size);

    const auto it = filtered_data.cbegin();
    const auto it_end = filtered_data.cend();
    const auto it_defiltered = defiltered_data.cbegin();
    const auto it_defiltered_end = defiltered_data.cend();

    /*!
     * As every scanline starts with an extra byte for the filter type, we must
     * sum the size of a scanline plus the extra byte.
    */
    for (uint32_t row = 0; row < filtered_data.size(); row += m_scanline_size + 1)
    {
        auto extra_filter_bytes_accumulated = (row / (m_scanline_size + 1));
        auto filtered_scanline_begin = filtered_data.begin() + row + 1;
        auto filtered_scanline_end = filtered_data.begin() + row + m_scanline_size + 1;
        auto defiltered_scanline_begin = defiltered_data.begin() + row - extra_filter_bytes_accumulated;

        if (not utils::isWithinBoundaries(it, it_end, filtered_scanline_begin, filtered_scanline_end)
            or not utils::isWithinBoundaries
            (
                it_defiltered,
                it_defiltered_end,
                static_cast<utils::Bytes::const_iterator>(defiltered_scanline_begin)
            )
        ) { throw std::out_of_range(std::string("Out of range iterators: ") + __func__); }

        auto filter_type = static_cast<uint8_t>(filtered_data[row]);
        auto previous_defiltered_scanline_begin = defiltered_data.cend();
        auto previous_defiltered_scanline_end = defiltered_data.cend();

        // It means we have a previous scanline
        if (row > 0)
        {
            previous_defiltered_scanline_begin = defiltered_scanline_begin - m_scanline_size;
            previous_defiltered_scanline_end = defiltered_scanline_begin + m_scanline_size;
        }

        /*!
         * Filters, differently from those applied from editing image software, serves the purpose
         * increasing the efficiency of zlib's deflate algorithm compression, as doing so, they reduce
         * the redundancy within and between scanlines (you will see why
         * when you look at filters that interact with previous pixels and scanlines) when processing the raw bytes.
         * By reducing redundancy filter allows for better compression which results in smaller file sizes, and the best of all
         * at no loss of information, what goes in, will go out, no loss whatsover.
         * Each scanline has its filter, a combination of all filters is made to better compress the entire image,
         * the result wouldn't be as good if just one filter was chosen for the entire image.
         *
         * Now if we were to encode images instead, how to choose between the available filters?
         * Tipically heuristics algorithms are used to decide,
         * these algorithms won't find the most optmized filter for each scanline,
         * instead they would find something that is "good enough" for efficient compression.
         * While I won’t cover these algorithms here, it’s important to note that the chosen filters are not random,
         * there’s a method to the selection process.
         *
         * Before explaining each filter and how to defilter them, there's some rules we must establish,
         * the filters' formula are fixed, meaning there are no different treatment when there's missing value,
         * for example, when a formula asks us to subtract the current pixel from the previous pixel,
         * what do we do in the case the current scanline has no previous pixel to start with?
         * We treat the previous pixel as 0, same applies for the above and upper left pixels, if there isn't any, they're 0.
         *
         * Another rule is that there's no negative value when filtering, so each operation which would cause the value
         * to go below zero, gets applied to modulus of (bit depth * number of channels per pixel), so it wraps around,
         * this information isn't useful for decoders, as the filter is already applied.
         *
         * TODO: Introduce explanation on a per bits basis for < 8 bits depth color.
         *
         * While I'm giving the formula in pixels, the processing happens on a per bytes basis,
         * current_channel_red_byte with previous_channel_red_byte, green with green, blue with blue, and so on,
         * if there's more than one byte (in case of 16 bits depth color) we continue the processing on a per byte basis,
         * works the same for the previous scanline too, we must align the channels before processing.
         *
         * -----------------------------------------------------------------------------------------------------------------
         *
         * None filter, there's nothing to do,
         * we simply leave the filtered scanline as is in the defiltered scanline.
         *
         * -----------------------------------------------------------------------------------------------------------------
         *
         * Sub filter formula:
         *
         * CurrentFilteredPixel(x) = CurrentRawPixel(x) - PreviousFilteredPixel(x)
         *
         * To defilter it, we simply do the inverse operation, instead of subtracting we sum.
         *
         * CurrentDefilteredPixel(x) = CurrentFilteredPixel(x) + PreviousDefilteredPixel(x)
         *
         * ---------
         * |---|---|
         * |PDP|CFP|
         * ---------
         *
         * If there's no previous defiltered pixel, it's 0, which leave the current filtered pixel as is.
         * In the case of this filter, the next processing will have a previous pixel to process so we must process
         * each pixel that follows.
         *
         * -----------------------------------------------------------------------------------------------------------------
         *
         * Up filter formula:
         *
         * CurrentFilteredPixel(x) = CurrentRawPixel(x) - AboveFilteredPixel(x)
         *
         * To the defilter it, do the inverse operation, we sum.
         *
         * CurrentDefilteredPixel(x) = CurrentFilteredPixel(x) + AboveDefilteredPixel(x)
         *
         * ---------
         * |---|ADP|
         * |---|CFP|
         * ---------
         *
         * If there's no above defiltered pixel, it's 0, as all pixels above will be zero, for the first defiltered scanline
         * for this filter, we leave the entire filtered scanline as is in the defiltered scanline.
         *
         * -----------------------------------------------------------------------------------------------------------------
         *
         * Average filter formula:
         *
         * CurrentFilteredPixel(x) = CurrentRawPixel(x) - rounded_down((AboveFilteredPixel(x) + PreviousFilteredPixel(x) / 2))
         *
         * To defilter, we sum the average instead of subtracting.
         *
         * CurrentDefilteredPixel(x) = CurrentFilteredPixel(x) + rounded_down((AboveDefilteredPixel(x) + PreviousDefilteredPixel(x) / 2))
         *
         * ---------
         * |---|ADP|
         * |PDP|CFP|
         * ---------
         *
         * If there's no above defiltered pixel, it's 0, as all pixels above will be zero, for the first defiltered scanline
         * for this filter, if there's no previous defiltered pixel, it's also 0.
         *
         * -----------------------------------------------------------------------------------------------------------------
         *
         * Paeth filter formula:
         *
         * CurrentFilteredPixel(x) =
         * CurrentRawPixel(x) - PaethPredictor(PreviousFilteredPixel(x), AboveFilteredPixel(x), UpperLeftFilteredPixel(x))
         *
         * To defilter instead of subtracting the result of the paeth predictor, we sum it.
         *
         * Paeth predictor algorithm pseudo code:
         *
         * InitialPredictor = Previous + Above - UpperLeft
         * PredictorPrevious = InitialPredictor - Previous
         * PredictorAbove = InitialPredictor - Above
         * PredictorUpperLeft = InitialPredictor - UpperLeft
         * FinalPredictor = Previous if PredictorPrevious <= PredictorAbove and PredictorPrevious <= PredictorUpperLeft
         *                  else Above if PredictorAbove <= PredictorUpperLeft
         *                  else UpperLeft
         *
         * ---------
         * |UAP|ADP|
         * |PDP|CFP|
         * ---------
        */
        switch (filter_type)
        {
            case NONE_FILTER_TYPE:
                std::copy
                (
                    filtered_scanline_begin,
                    filtered_scanline_end,
                    defiltered_scanline_begin
                );
                break;
            case SUB_FILTER_TYPE:
                defilterSubFilter
                (
                    filtered_scanline_begin,
                    filtered_scanline_end,
                    defiltered_scanline_begin
                );
                break;
            case UP_FILTER_TYPE:
                defilterUpFilter
                (
                    filtered_scanline_begin,
                    filtered_scanline_end,
                    previous_defiltered_scanline_begin,
                    previous_defiltered_scanline_end,
                    defiltered_scanline_begin
                );
                break;
            case AVERAGE_FILTER_TYPE:
                defilterAverageFilter
                (
                    filtered_scanline_begin,
                    filtered_scanline_end,
                    previous_defiltered_scanline_begin,
                    previous_defiltered_scanline_end,
                    defiltered_scanline_begin
                );
                break;
            case PAETH_FILTER_TYPE:
                defilterPaethFilter
                (
                    filtered_scanline_begin,
                    filtered_scanline_end,
                    previous_defiltered_scanline_begin,
                    previous_defiltered_scanline_end,
                    defiltered_scanline_begin
                );
                break;
            default:
                // This should never happen
                throw std::runtime_error("Filter mode is invalid.\n");
                break;
        };
    }
} // Scalines::defilterData

void Scanlines::defilterSubFilter
(
    CScanlineBegin filtered_scanline_begin,
    CScanlineEnd filtered_scanline_end,
    ScanlineBegin defiltered_scanline_begin
)
{
    uint8_t current {0};
    uint8_t left_of_current {0};

    /*!
     * Remember the rule, if there isn't a previous pixel, it's 0,
     * this would be the equivalent of doing (current pixel + 0), which would be the current pixel itself.
     *
     * So we copy the first pixel as is to our unfiltered scanline.
    */
    std::copy
    (
        filtered_scanline_begin,
        filtered_scanline_begin + m_bytes_per_pixel,
        defiltered_scanline_begin
    );

    /*!
     * We already processed the "current pixel", then point to the start of the "next pixel".
    */
    filtered_scanline_begin += m_bytes_per_pixel;
    defiltered_scanline_begin += m_bytes_per_pixel;

    for (;filtered_scanline_begin != filtered_scanline_end; ++filtered_scanline_begin, ++defiltered_scanline_begin)
    {
        current = static_cast<uint8_t>(*filtered_scanline_begin);
        left_of_current = static_cast<uint8_t>(*(defiltered_scanline_begin - m_bytes_per_pixel));
        *defiltered_scanline_begin = utils::Byte(current + left_of_current);
    }
} // Scalines::defilterSubFilter

void Scanlines::defilterUpFilter(
    CScanlineBegin filtered_scanline_begin,
    CScanlineEnd filtered_scanline_end,
    CScanlineBegin previous_defiltered_scanline_begin,
    CScanlineEnd previous_defiltered_scanline_end,
    ScanlineBegin defiltered_scanline_begin
)
{
    uint8_t current {0};
    uint8_t above_current {0};

    if (previous_defiltered_scanline_begin == previous_defiltered_scanline_end)
    {
        /*!
         * If there's no previous pixel or scanline, assume it's 0,
         * which for defiltering the Up filter, is just the filtered scanline itself.
         *
         * CurrentDefilteredPixel(x) = CurrentFilteredPixel(x) + CurrentDefilteredPixelAbove(x),
         *
         * CurrentDefilteredPixelAbove(x) will always be 0, so we leave the entire scanline as is.
        */
        std::copy(filtered_scanline_begin, filtered_scanline_end, defiltered_scanline_begin);
        return;
    }

    for (;filtered_scanline_begin != filtered_scanline_end;
        ++filtered_scanline_begin,
        ++defiltered_scanline_begin,
        ++previous_defiltered_scanline_begin
    )
    {
        current = static_cast<uint8_t>(*filtered_scanline_begin);
        above_current = static_cast<uint8_t>(*(previous_defiltered_scanline_begin));
        *defiltered_scanline_begin = std::byte(current + above_current);
    }
} // Scalines::defilterUpFilter

void Scanlines::defilterAverageFilter
(
    CScanlineBegin filtered_scanline_begin,
    CScanlineEnd filtered_scanline_end,
    CScanlineBegin previous_defiltered_scanline_begin,
    CScanlineEnd previous_defiltered_scanline_end,
    ScanlineBegin defiltered_scanline_begin
)
{
    uint8_t current {0};
    uint8_t left_of_current {0};
    uint8_t above_current {0};

    if (previous_defiltered_scanline_begin == previous_defiltered_scanline_end)
    {
        /*!
         * The formula to defilter average filter is:
         *
         * CurrentDefilteredPixel(x) = CurrentFilteredPixel(x) + rounded_down(PreviousDefilteredPixel(x) + AboveDefilteredPixel(x) / 2)
         *
         * If there's no previous scanline, it's 0, and as we just starting to process this scanline,
         * there isn't any previous pixel yet. The above would expand to:
         *
         * CurrentDefilteredPixel(x) = CurrentFilteredPixel(x) + rounded_down(0 + 0 / 2)
         * CurrentDefilteredPixel(x) = CurrentFilteredPixel(x) + 0
         * CurrentDefilteredPixel(x) = CurrentFilteredPixel(x)
         *
         * In other words we just copy the pixel as is to the current scanline being processed.
        */

        std::copy
        (
            filtered_scanline_begin,
            filtered_scanline_begin + m_bytes_per_pixel,
            defiltered_scanline_begin
        );

        /*!
         * Point to the next pixel.
        */
        filtered_scanline_begin += m_bytes_per_pixel;
        defiltered_scanline_begin += m_bytes_per_pixel;

        for (;filtered_scanline_begin != filtered_scanline_end; ++filtered_scanline_begin, ++defiltered_scanline_begin)
        {
            current = static_cast<uint8_t>(*filtered_scanline_begin);
            left_of_current = static_cast<uint8_t>(*(defiltered_scanline_begin - m_bytes_per_pixel));
            *defiltered_scanline_begin = utils::Byte(current + std::floor(left_of_current / 2));
        }

        return;
    }

    /*!
     * This handles the case where we have a previous scanline, but not a previous pixel yet,
     * differently from the Sub filter, we can't simply copy the first pixel as is,
     * the operation doesn't need only the previous pixel, but also the above to the average and we have that.
     *
     * So it becomes:
     *
     * CurrentDefilteredPixel(x) = CurrentFilteredPixel(x) + rounded_down(0 + AboveDefilteredPixel(x) / 2)
     * CurrentDefilteredPixel(x) = CurrentFilteredPixel(x) + rounded_down(AboveDefilteredPixel(x) / 2)
     *
    */

    auto current_pixel_end = filtered_scanline_begin + m_bytes_per_pixel;

    for (
        ;filtered_scanline_begin != current_pixel_end;
        ++filtered_scanline_begin,
        ++defiltered_scanline_begin,
        ++previous_defiltered_scanline_begin
    )
    {
        current = static_cast<uint8_t>(*filtered_scanline_begin);
        above_current = static_cast<uint8_t>(*previous_defiltered_scanline_begin);
        *defiltered_scanline_begin = utils::Byte(current + std::floor(above_current / 2));
    }

    /*!
     * The above had the iterators pointing to the next pixel already.
    */
    for (
        ;filtered_scanline_begin != filtered_scanline_end;
        ++filtered_scanline_begin,
        ++defiltered_scanline_begin,
        ++previous_defiltered_scanline_begin
    )
    {
        current = static_cast<uint8_t>(*filtered_scanline_begin);
        left_of_current = static_cast<uint8_t>(*(defiltered_scanline_begin - m_bytes_per_pixel));
        above_current = static_cast<uint8_t>(*previous_defiltered_scanline_begin);
        *defiltered_scanline_begin = utils::Byte(current + std::floor((left_of_current + above_current) / 2));
    }
} // Scanlines::defilterAverageFilter

void Scanlines::defilterPaethFilter
(
    CScanlineBegin filtered_scanline_begin,
    CScanlineEnd filtered_scanline_end,
    CScanlineBegin previous_defiltered_scanline_begin,
    CScanlineEnd previous_defiltered_scanline_end,
    ScanlineBegin defiltered_scanline_begin
)
{
    uint8_t current {0};
    uint8_t left_of_current {0};
    uint8_t above_current {0};
    uint8_t upper_left_of_current {0};

    /*!
     * The paeth filter formula goes as:
     *
     * CurrentFilteredPixel(x) =
     * CurrentRawPixel(x) - PaethPredictor(PreviousFilteredPixel(x), AboveFilteredPixel(x), UpperLeftFilteredPixel(x))
     *
     * The formula to revert the paeth filter is:
     *
     * CurrentDefilteredPixel(x) =
     * CurrentFilteredPixel(x) + PaethPredictor(PreviousDefilteredPixel(x), AboveDefilteredPixel(x), UpperLeftDefilteredPixel(x))
     *
    */

    /*!
     * When there's no previous scanline, there won't be above or upper left pixels, they will be 0, the property of the
     * paeth predictor is that it will return the non-zero value provided to it, as long as the other two values are 0.
     * In the case above, where there's no previous scanline, it would always return the value of the left pixel.
    */
    if (previous_defiltered_scanline_begin == previous_defiltered_scanline_end)
    {
        /*!
         * Here is as if the left, above and upper left were 0, if we passed all zeroes to the paeth predictor,
         * it would return 0, summing the current filtered pixel to zero would result in the filtered pixel itself,
         * so we copy it as is.
        */
        std::copy(filtered_scanline_begin, filtered_scanline_begin + m_bytes_per_pixel, defiltered_scanline_begin);

        filtered_scanline_begin += m_bytes_per_pixel;
        defiltered_scanline_begin += m_bytes_per_pixel;

        for (
            ;filtered_scanline_begin != filtered_scanline_end;
            ++filtered_scanline_begin,
            ++defiltered_scanline_begin
        )
        {
            current = static_cast<uint8_t>(*filtered_scanline_begin);
            left_of_current = static_cast<uint8_t>(*(defiltered_scanline_begin - m_bytes_per_pixel));
            *defiltered_scanline_begin = utils::Byte(current + left_of_current);
        }

        return;
    }

    /*!
     * This is the case where we have a previous scanline, but as we are at the beginning of it,
     * there's no previous pixel for the current filtered pixel and there's no upper left pixel for the current filtered pixel,
     * but there's a defiltered pixel above the current filtered pixel.
     *
     * Passing this above pixel to the paeth predictor, and zero for the other values, would return the above pixel itself,
     * so we just sum the current filtered pixel to this defiltered pixel above.
    */
    auto current_pixel_end = filtered_scanline_begin + m_bytes_per_pixel;

    for (
        ;filtered_scanline_begin != current_pixel_end;
        ++filtered_scanline_begin,
        ++previous_defiltered_scanline_begin,
        ++defiltered_scanline_begin
    ){
        current = static_cast<uint8_t>(*filtered_scanline_begin);
        above_current = static_cast<uint8_t>(*previous_defiltered_scanline_begin);
        *defiltered_scanline_begin = utils::Byte(current + above_current);
    }

    for (
        ;filtered_scanline_begin != filtered_scanline_end;
        ++filtered_scanline_begin,
        ++previous_defiltered_scanline_begin,
        ++defiltered_scanline_begin
    )
    {
        current = static_cast<uint8_t>(*filtered_scanline_begin);
        left_of_current = static_cast<uint8_t>(*(defiltered_scanline_begin - m_bytes_per_pixel));
        above_current = static_cast<uint8_t>(*previous_defiltered_scanline_begin);
        upper_left_of_current = static_cast<uint8_t>(*(previous_defiltered_scanline_begin - m_bytes_per_pixel));
        *defiltered_scanline_begin = utils::Byte(current + paethPredictor(left_of_current, above_current, upper_left_of_current));
    }
} // Scanlines::defilterPaethFilter


uint8_t Scanlines::paethPredictor
(
    int left_of_current,
    int above_current,
    int upper_left_of_current
) const noexcept
{
    int p = left_of_current + above_current - upper_left_of_current;
    int p_left_of_current = std::abs(p - left_of_current);
    int p_above_current = std::abs(p - above_current);
    int p_upper_left_of_current = std::abs(p - upper_left_of_current);

    if (p_left_of_current <= p_above_current and p_left_of_current <= p_upper_left_of_current)
        return left_of_current;
    else if (p_above_current <= p_upper_left_of_current)
        return above_current;
    else
        return upper_left_of_current;
} // Scalines::paethPredictor
} // namespace image_formats::png_format
