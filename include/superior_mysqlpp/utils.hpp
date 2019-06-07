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
        auto invokeViaTupleImpl(Function f, Tuple &&t, std::index_sequence<I...>)
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
    auto invokeViaTuple(Function f, Tuple &&t)
    {
        constexpr std::size_t tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;
        return detail::invokeViaTupleImpl(f, std::forward<Tuple>(t), std::make_index_sequence<tuple_size>{});
    }

    /**
     * Writes variable number of arguments into arbitrary std(-like) stream.
     * @param stream Stream reference.
     * @param arg Perfectly forwarded argument
     * @return Stream reference
     */
    template <typename Stream, typename Arg>
    Stream &streamify(Stream &stream, Arg &&arg)
    {
        return stream << std::forward<Arg>(arg);
    }

    /**
     * @overload
     */
    template <typename Stream, typename Arg, typename... Args>
    Stream &streamify(Stream &stream, Arg &&arg, Args&&...args)
    {
        return streamify(streamify(stream, arg), std::forward<Args>(args)...);
    }

    namespace detail
    {
        /**
         * Detects, if all template parameters are references
         * 
         * @tparam Args... Types to be checked
         */
        template<typename... Args>
        struct are_arguments_lrefs : std::false_type {};

        /**
         * Detects, if all template parameters are references
         *
         * @tparam Args... Types to be checked
         */
        template<typename... Args>
        struct are_arguments_lrefs<Args&...> : std::true_type {};

        /**
         * @brief Generic function info obtained from FunctionInfo trait
         * 
         * @tparam ResultType type returned by function
         * @tparam IsConst true_type, if function has been marked const
         * @tparam IsVolatile true_type, if function has been marked volatile
         * @tparam Args function arguments
         */
        template<typename ResultType, typename IsConst, typename IsVolatile, typename... Args>
        struct FunctionInfoImpl
        {
            using result_type   = ResultType;
            using is_const      = IsConst;
            using is_volatile   = IsVolatile;
            using arguments     = std::tuple<Args...>;
            using raw_arguments = std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...>;

            using arguments_are_lvalue_references = are_arguments_lrefs<Args...>;
        };
    }

    /**
     * Returns information about function
     * 
     * @tparam T function prototype
     */
    template<typename T>
    struct FunctionInfo;

    /**
     * @brief information about std::function<>-like declaration (e.g. `void(int, int)`)
     * 
     * @tparam ResultType type returned by function
     * @tparam Args function arguments
     */
    template<typename ResultType, typename... Args>
    struct FunctionInfo<ResultType (Args...)> : detail::FunctionInfoImpl<ResultType, std::false_type, std::false_type, Args...> {};

    /**
     * @brief Plain old C function overload
     * 
     * @tparam ResultType type returned by function
     * @tparam Args function arguments
     */
    template<typename ResultType, typename... Args>
    struct FunctionInfo<ResultType (*)(Args...)> : detail::FunctionInfoImpl<ResultType, std::false_type, std::false_type, Args...> {};

    /**
     * @brief Class member function overload
     * 
     * @tparam Class Class of member
     * @tparam ResultType type returned by function
     * @tparam Args function arguments
     */
    template<typename Class, typename ResultType, typename... Args>
    struct FunctionInfo<ResultType (Class::*)(Args...)> : detail::FunctionInfoImpl<ResultType, std::false_type, std::false_type, Args...> {};

    /**
     * @brief Class const-member function overload
     * 
     * @tparam Class Class of member
     * @tparam ResultType type returned by function
     * @tparam Args function arguments
     */
    template<typename Class, typename ResultType, typename... Args>
    struct FunctionInfo<ResultType (Class::*)(Args...) const> : detail::FunctionInfoImpl<ResultType, std::true_type, std::false_type, Args...> {};

    /**
     * @brief Class volatile-member function overload
     * 
     * @tparam Class Class of member
     * @tparam ResultType type returned by function
     * @tparam Args function arguments
     */
    template<typename Class, typename ResultType, typename... Args>
    struct FunctionInfo<ResultType (Class::*)(Args...) volatile> : detail::FunctionInfoImpl<ResultType, std::false_type, std::true_type, Args...> {};

    /**
     * @brief Class const-volatile-member function overload
     * 
     * @tparam Class Class of member
     * @tparam ResultType type returned by function
     * @tparam Args function arguments
     */
    template<typename Class, typename ResultType, typename... Args>
    struct FunctionInfo<ResultType (Class::*)(Args...) const volatile> : detail::FunctionInfoImpl<ResultType, std::true_type, std::true_type, Args...> {};

    /**
     * @brief Lambda overload (by decaying its type to class member function)
     * 
     * @tparam Class Class of member
     * @tparam ResultType type returned by function
     * @tparam Args function arguments
     */
    template<typename T>
    struct FunctionInfo : FunctionInfo<decltype(&std::decay_t<T>::operator())> {};
}
