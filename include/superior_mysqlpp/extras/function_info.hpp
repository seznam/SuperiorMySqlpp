#pragma once
#include <tuple>
#include <type_traits>

namespace SuperiorMySqlpp
{
    namespace detail
    {
        /**
         * Detects, if all template parameters are references
         */
        template<typename...>
        struct are_arguments_lrefs;

        template<typename Arg, typename... Args>
        struct are_arguments_lrefs<Arg, Args...> : std::conditional<std::is_lvalue_reference<Arg>::value, are_arguments_lrefs<Args...>, std::false_type>::type {};

        template<>
        struct are_arguments_lrefs<> : std::true_type {};

        /**
         * Generic function info obtained from FunctionInfo trait
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
     */
    template<typename>
    struct FunctionInfo;

    template<typename ResultType, typename... Args>
    struct FunctionInfo<ResultType (Args...)> : detail::FunctionInfoImpl<ResultType, std::false_type, std::false_type, Args...> {};

    template<typename ResultType, typename... Args>
    struct FunctionInfo<ResultType (*)(Args...)> : detail::FunctionInfoImpl<ResultType, std::false_type, std::false_type, Args...> {};

    template<typename Class, typename ResultType, typename... Args>
    struct FunctionInfo<ResultType (Class::*)(Args...)> : detail::FunctionInfoImpl<ResultType, std::false_type, std::false_type, Args...> {};

    template<typename Class, typename ResultType, typename... Args>
    struct FunctionInfo<ResultType (Class::*)(Args...) const> : detail::FunctionInfoImpl<ResultType, std::true_type, std::false_type, Args...> {};

    template<typename Class, typename ResultType, typename... Args>
    struct FunctionInfo<ResultType (Class::*)(Args...) volatile> : detail::FunctionInfoImpl<ResultType, std::false_type, std::true_type, Args...> {};

    template<typename Class, typename ResultType, typename... Args>
    struct FunctionInfo<ResultType (Class::*)(Args...) const volatile> : detail::FunctionInfoImpl<ResultType, std::true_type, std::true_type, Args...> {};

    template<typename T>
    struct FunctionInfo : FunctionInfo<decltype(&std::decay_t<T>::operator())> {};
}