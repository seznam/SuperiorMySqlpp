/*
 * Author: Tomas Nozicka
 */

#pragma once


#include <type_traits>



namespace SuperiorMySqlpp
{
    namespace detail
    {
        enum class BindingTypes
        {
            Nullable,
            String,
            Decimal,
            Blob,
            Date,
            Time,
            Datetime,
            Timestamp,
        };


        template<BindingTypes, typename T>
        struct CanBindAsParam : std::false_type {};

        template<BindingTypes, typename T>
        struct CanBindAsResult: std::false_type {};
    }
}
