/*
 * Author: Tomas Nozicka
 */

#pragma once


#include <string>
#include <tuple>
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

    namespace detail
    {
        /**
         * @brief Invokes function via unpacked tuple
         * @param f Functor to be invoked
         * @param t Tuple of arguments to be unpacked
         */
        template<typename Function, typename Tuple, std::size_t... I>
        auto invokeViaTupleImpl(Function f, const Tuple &t, std::index_sequence<I...>)
        {
            return f(std::get<I>(t)...);
        }
    }

    /**
     * @brief Invokes function via unpacked tuple
     * @param f Functor to be invoked
     * @param t Tuple of arguments to be unpacked
     */
    template<typename Function, typename Tuple>
    auto invokeViaTuple(Function f, const Tuple &t)
    {
        constexpr std::size_t tuple_size = std::tuple_size<Tuple>::value;
        return detail::invokeViaTupleImpl(f, t, std::make_index_sequence<tuple_size>{});
    }
}
