/*
 * Author: Tomas Nozicka
 */

#pragma once


#include <tuple>


namespace SuperiorMySqlpp
{
    /*
     * initializeResultItem
     */
    template<typename DecayedType>
    struct InitializeResultItemImpl
    {
        template<typename T>
        static void call(T&&)
        {
        }
    };

    template<typename T>
    void intializeResultItem(T&& value)
    {
        InitializeResultItemImpl<std::decay_t<T>>::call(value);
    }



    namespace detail
    {
        /*
         * Initialize bindings implementation
         */
        template<int N>
        struct InitializeResultImpl
        {
            template<typename Data>
            static void call(Data& data)
            {
                intializeResultItem(std::get<N-1>(data));
                InitializeResultImpl<N-1>::call(data);
            }
        };

        template<>
        struct InitializeResultImpl<0>
        {
            template<typename Data>
            static void call(Data&)
            {
            }
        };
    }


    /*
     * Initialize Bindings
     */
    template<typename... Types>
    void InitializeResult(std::tuple<Types...>& data)
    {
        detail::InitializeResultImpl<sizeof...(Types)>::call(data);
    }
}


