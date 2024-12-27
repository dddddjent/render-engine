#pragma once

#define DEFINE_ENUM_BIT_OPERATORS(T)                                      \
    inline constexpr T operator|(const T lhs, const T rhs)                \
    {                                                                     \
        using Underlying = std::underlying_type_t<T>;                     \
        return static_cast<T>(                                            \
            static_cast<Underlying>(lhs) | static_cast<Underlying>(rhs)); \
    }                                                                     \
                                                                          \
    inline constexpr T operator&(const T lhs, const T rhs)                \
    {                                                                     \
        using Underlying = std::underlying_type_t<T>;                     \
        return static_cast<T>(                                            \
            static_cast<Underlying>(lhs) & static_cast<Underlying>(rhs)); \
    }
