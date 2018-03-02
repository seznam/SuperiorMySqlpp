/*
 * Author: Tomas Nozicka
 */

#pragma once


#include <string>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace SuperiorMySqlpp
{
    template<typename T, template <typename, typename=std::allocator<T>> class Container>
    inline std::string toString(Container<T> container)
    {
        std::string result{};
        std::size_t i= 0;
        for (auto&& item: container)
        {
            result += std::to_string(item);
            if (++i != container.size())
            {
                result += ", ";
            }
        }

        return result;
    }

    // !!!!! Move to traits.hpp?
    namespace detail {
        template <typename T1,typename T2>
        struct is_same_int_type {
            static_assert(std::is_integral<T1>::value && std::is_integral<T2>::value,"Only integer types allowed!");
            static constexpr bool value = (sizeof(T1) == sizeof(T2)) && (std::is_unsigned<T1>::value == std::is_unsigned<T2>::value);
        };

        template <typename Type,typename Enabled = void>
        struct to_fixed_width_int_impl {
        };
        template <typename T>
        struct to_fixed_width_int_impl<T,std::enable_if_t<is_same_int_type<T,int8_t>::value>> {
            using type = int8_t;
        };
        template <typename T>
        struct to_fixed_width_int_impl<T,std::enable_if_t<is_same_int_type<T,uint8_t>::value>> {
            using type = uint8_t;
        };
        template <typename T>
        struct to_fixed_width_int_impl<T,std::enable_if_t<is_same_int_type<T,int16_t>::value>> {
            using type = int16_t;
        };
        template <typename T>
        struct to_fixed_width_int_impl<T,std::enable_if_t<is_same_int_type<T,uint16_t>::value>> {
            using type = uint16_t;
        };
        template <typename T>
        struct to_fixed_width_int_impl<T,std::enable_if_t<is_same_int_type<T,int32_t>::value>> {
            using type = int32_t;
        };
        template <typename T>
        struct to_fixed_width_int_impl<T,std::enable_if_t<is_same_int_type<T,uint32_t>::value>> {
            using type = uint32_t;
        };
        template <typename T>
        struct to_fixed_width_int_impl<T,std::enable_if_t<is_same_int_type<T,int64_t>::value>> {
            using type = int64_t;
        };
        template <typename T>
        struct to_fixed_width_int_impl<T,std::enable_if_t<is_same_int_type<T,uint64_t>::value>> {
            using type = uint64_t;
        };
    }

    /**
     * Turns integer type T (notably basic built-in architecture specific range types)
     * to corresponding exact bit width type.
     * Type trait / type metafunction - result is type in member ::type.
     * Note that this requires architecture that supports intXX_t types.
     */
    template <typename T>
    using to_fixed_width_int = detail::to_fixed_width_int_impl<T>;

    /**
     * Convenience alias for to_fixed_width_int.
     */
    template <typename T>
    using to_fixed_width_int_t = typename to_fixed_width_int<T>::type;
}
