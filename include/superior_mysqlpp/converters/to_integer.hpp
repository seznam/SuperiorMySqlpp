/*
 * Author: Tomas Nozicka
 */

#pragma once


#include <cinttypes>
#include <utility>
#include <limits>
#include <stdexcept>


namespace SuperiorMySqlpp { namespace Converters
{
    namespace detail
    {
        template <class T>
        inline constexpr T pow(T const& x, std::size_t n) {
            return n > 0 ? x * pow(x, n - 1) : 1;
        }

        template<typename T, int length, bool validate>
        struct ToIntegerParserImpl
        {
            static_assert(std::numeric_limits<T>::digits10, "length > digits10");
            static inline void call(T& result, const char*& str) noexcept(!validate)
            {
                static constexpr T base = pow(10UL, length-1);
                char c = *str;
                if (validate)
                {
                    if (c < '0' || c > '9')
                    {
                        throw std::out_of_range("Character is not between 0 and 9!");
                    }
                }
                result += (c - '0') * base;
                ++str;
                ToIntegerParserImpl<T, length-1, validate>::call(result, str);
            }
        };

        template<typename T, bool validate>
        struct ToIntegerParserImpl<T, 1, validate>
        {
            static_assert(std::numeric_limits<T>::digits10, "length > digits10");
            static inline void call(T& result, const char*& str) noexcept(!validate)
            {
                char c = *str;
                if (validate)
                {
                    if (c < '0' || c > '9')
                    {
                        throw std::out_of_range("Character is not between 0 and 9!");
                    }
                }
                result += (c - '0');
                ++str;
            }
        };


        template<typename T, bool validate>
        struct ToIntegerUnsignedImpl
        {
            static inline T call(const char* str, unsigned int length) = delete;
        };

        template<bool validate>
        struct ToIntegerUnsignedImpl<std::uint8_t, validate>
        {
            static inline std::uint8_t call(const char* str, unsigned int length)
            {
                std::uint8_t result = 0;
                switch (length)
                {
                    case 3:
                        ToIntegerParserImpl<decltype(result), 3, validate>::call(result, str);
                        break;
                    case 2:
                        ToIntegerParserImpl<decltype(result), 2, validate>::call(result, str);
                        break;
                    case 1:
                        ToIntegerParserImpl<decltype(result), 1, validate>::call(result, str);
                        break;
                    default:
                        throw std::runtime_error("Internal error!");
                }
                return result;
            }
        };

        template<bool validate>
        struct ToIntegerUnsignedImpl<std::uint16_t, validate>
        {
            static inline std::uint16_t call(const char* str, unsigned int length)
            {
                std::uint16_t result = 0;
                switch (length)
                {
                    case 5:
                        ToIntegerParserImpl<decltype(result), 5, validate>::call(result, str);
                        break;
                    case 4:
                        ToIntegerParserImpl<decltype(result), 4, validate>::call(result, str);
                        break;
                    case 3:
                        ToIntegerParserImpl<decltype(result), 3, validate>::call(result, str);
                        break;
                    case 2:
                        ToIntegerParserImpl<decltype(result), 2, validate>::call(result, str);
                        break;
                    case 1:
                        ToIntegerParserImpl<decltype(result), 1, validate>::call(result, str);
                        break;
                    default:
                        throw std::runtime_error("Internal error!");
                }
                return result;
            }
        };

        template<bool validate>
        struct ToIntegerUnsignedImpl<std::uint32_t, validate>
        {
            static inline std::uint32_t call(const char* str, unsigned int length)
            {
                std::uint32_t result = 0;
                switch (length)
                {
                    case 10:
                        ToIntegerParserImpl<decltype(result), 10, validate>::call(result, str);
                        break;
                    case 9:
                        ToIntegerParserImpl<decltype(result), 9, validate>::call(result, str);
                        break;
                    case 8:
                        ToIntegerParserImpl<decltype(result), 8, validate>::call(result, str);
                        break;
                    case 7:
                        ToIntegerParserImpl<decltype(result), 7, validate>::call(result, str);
                        break;
                    case 6:
                        ToIntegerParserImpl<decltype(result), 6, validate>::call(result, str);
                        break;
                    case 5:
                        ToIntegerParserImpl<decltype(result), 5, validate>::call(result, str);
                        break;
                    case 4:
                        ToIntegerParserImpl<decltype(result), 4, validate>::call(result, str);
                        break;
                    case 3:
                        ToIntegerParserImpl<decltype(result), 3, validate>::call(result, str);
                        break;
                    case 2:
                        ToIntegerParserImpl<decltype(result), 2, validate>::call(result, str);
                        break;
                    case 1:
                        ToIntegerParserImpl<decltype(result), 1, validate>::call(result, str);
                        break;
                    default:
                        throw std::runtime_error("Internal error!");
                }
                return result;
            }
        };

        template<bool validate>
        struct ToIntegerUnsignedImpl<std::uint64_t, validate>
        {
            static inline std::uint64_t call(const char* str, unsigned int length)
            {
                std::uint64_t result = 0;
                switch (length)
                {
                    case 20:
                        ToIntegerParserImpl<decltype(result), 20, validate>::call(result, str);
                        break;
                    case 19:
                        ToIntegerParserImpl<decltype(result), 19, validate>::call(result, str);
                        break;
                    case 18:
                        ToIntegerParserImpl<decltype(result), 18, validate>::call(result, str);
                        break;
                    case 17:
                        ToIntegerParserImpl<decltype(result), 17, validate>::call(result, str);
                        break;
                    case 16:
                        ToIntegerParserImpl<decltype(result), 16, validate>::call(result, str);
                        break;
                    case 15:
                        ToIntegerParserImpl<decltype(result), 15, validate>::call(result, str);
                        break;
                    case 14:
                        ToIntegerParserImpl<decltype(result), 14, validate>::call(result, str);
                        break;
                    case 13:
                        ToIntegerParserImpl<decltype(result), 13, validate>::call(result, str);
                        break;
                    case 12:
                        ToIntegerParserImpl<decltype(result), 12, validate>::call(result, str);
                        break;
                    case 11:
                        ToIntegerParserImpl<decltype(result), 11, validate>::call(result, str);
                        break;
                    case 10:
                        ToIntegerParserImpl<decltype(result), 10, validate>::call(result, str);
                        break;
                    case 9:
                        ToIntegerParserImpl<decltype(result), 9, validate>::call(result, str);
                        break;
                    case 8:
                        ToIntegerParserImpl<decltype(result), 8, validate>::call(result, str);
                        break;
                    case 7:
                        ToIntegerParserImpl<decltype(result), 7, validate>::call(result, str);
                        break;
                    case 6:
                        ToIntegerParserImpl<decltype(result), 6, validate>::call(result, str);
                        break;
                    case 5:
                        ToIntegerParserImpl<decltype(result), 5, validate>::call(result, str);
                        break;
                    case 4:
                        ToIntegerParserImpl<decltype(result), 4, validate>::call(result, str);
                        break;
                    case 3:
                        ToIntegerParserImpl<decltype(result), 3, validate>::call(result, str);
                        break;
                    case 2:
                        ToIntegerParserImpl<decltype(result), 2, validate>::call(result, str);
                        break;
                    case 1:
                        ToIntegerParserImpl<decltype(result), 1, validate>::call(result, str);
                        break;
                    default:
                        throw std::runtime_error("Internal error!");
                }
                return result;
            }
        };

        template<typename T, bool validate>
        inline T toIntegerUnsigned(const char* str, unsigned int length)
        {
            return ToIntegerUnsignedImpl<T, validate>::call(str, length);
        }
    }


    template<typename T, bool validate=false>
    inline std::enable_if_t<std::is_unsigned<T>::value, T> toInteger(const char* str, unsigned int length)
    {
        return detail::toIntegerUnsigned<T, validate>(str, length);
    }

    template<typename T, bool validate=false>
    inline std::enable_if_t<std::is_signed<T>::value, T> toInteger(const char* str, unsigned int length)
    {
        bool isNegative = false;
        if (*str == '-')
        {
            isNegative = true;
            ++str;
            --length;
        }

        T result = toInteger<std::make_unsigned_t<T>, validate>(str, length);
        if (isNegative)
        {
            return result * -1;
        }
        else
        {
            return result;
        }
    }
}}
