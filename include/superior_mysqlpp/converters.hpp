/*
 * Author: Tomas Nozicka
 */

#pragma once


#include <type_traits>

#include <superior_mysqlpp/converters/to_integer.hpp>
#include <superior_mysqlpp/converters/to_floating_point.hpp>



namespace SuperiorMySqlpp { namespace Converters
{
    template<typename T>
    inline std::enable_if_t<std::is_integral<T>::value, T> to(const char* str, unsigned int length)
    {
        return toInteger<T>(str, length);
    }

    template<typename T>
    inline std::enable_if_t<std::is_floating_point<T>::value, T> to(const char* str, unsigned int length)
    {
        return toFloatingPoint<T>(str, length);
    }

    template<typename T>
    inline std::enable_if_t<std::is_same<T, std::string>::value, T> to(const char* str, unsigned int length)
    {
        return {str, length};
    }

    template<typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type=0>
    inline std::string toString(T value)
    {
        return std::to_string(value);
    }

    template<typename T, typename std::enable_if<std::is_same<std::decay_t<T>, std::string>::value, int>::type=0>
    inline std::string toString(T&& value)
    {
        return std::forward<T>(value);
    }
}}
