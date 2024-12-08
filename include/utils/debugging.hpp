#pragma once

#include <bit>
#include <iostream>

namespace debugging
{
/*!
 * Just a mock up of a debugging classes to get quick runtime feedback,
 * instead of having a preprocessor variable to enable/disable logging.
 *
 * This file must be guarded to be added for debugging purposes only,
 * so there shouldn't be much of an issue.
*/

template<typename T>
class DebugAllocator
{
public:

    using value_type = T;

    DebugAllocator() = default;

    template<typename U>
    constexpr DebugAllocator(const DebugAllocator<U>&) noexcept {}

public:
    static void enableLogging()
    {
        m_enable_logging = true;
    }

    static void disableLogging()
    {
        m_enable_logging = false;
    }

    T* allocate(std::size_t n)
    {
        T* ptr = std::allocator<T>{}.allocate(n);

        if (m_enable_logging)
        {
            std::cout
            << "Allocating "
            << n
            << " objects ("
            << n * sizeof(T)
            << " bytes) at address "
            << std::bit_cast<void*>(ptr)
            << "\n";
        }

        return ptr;
    }

    void deallocate(T* ptr, std::size_t n) noexcept
    {
        if (m_enable_logging)
        {
            std::cout
            << "Deallocating "
            << n
            << " objects at address "
            << std::bit_cast<void*>(ptr)
            << "\n";
        }

        std::allocator<T>{}.deallocate(ptr, n);
    }

private:
    static bool m_enable_logging;
};

template <typename T>
bool DebugAllocator<T>::m_enable_logging = false;

template <typename T, typename U>
bool operator==(const DebugAllocator<T>&, const DebugAllocator<U>&) { return true; }

template<typename T, typename U>
bool operator!=(const DebugAllocator<T>&, const DebugAllocator<U>&) { return false; }

} // namespace debugging
