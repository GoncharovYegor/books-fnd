#pragma once

#include <cstddef>
#include <iterator>

#if defined(_MSC_VER) && _MSC_VER >= 1950
namespace stdext {
    template <typename T>
    using checked_array_iterator = T*;

    template <typename T>
    constexpr T* make_checked_array_iterator(T* ptr, std::size_t) noexcept {
        return ptr;
    }
} // namespace stdext
#endif
