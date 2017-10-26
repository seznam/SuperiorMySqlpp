/*
 * Author: Tomas Nozicka
 */

#pragma once


#include <superior_mysqlpp/prepared_statements/get_binding_type.hpp>
#include <superior_mysqlpp/prepared_statements/binding_types.hpp>
#include <superior_mysqlpp/types/blob_data.hpp>
#include <superior_mysqlpp/types/string_data.hpp>
#include <superior_mysqlpp/types/decimal_data.hpp>
#include <superior_mysqlpp/types/nullable.hpp>

namespace SuperiorMySqlpp
{
    namespace detail
    {
        template<>
        struct CanBindAsParam<BindingTypes::String, std::string> : std::true_type {};


        /*
         * Common bindings
         */
        template<typename T>
        constexpr inline void initializeNullable(MYSQL_BIND& binding, Nullable<T>& nullable)
        {
            static_assert(sizeof(decltype(binding.is_null)) == sizeof(decltype(&nullable.detail_getNullRef())), "");
            binding.is_null = reinterpret_cast<my_bool*>(&nullable.detail_getNullRef());
        }

        /*
         * Param bindings
         */
        template<typename T>
        constexpr inline std::enable_if_t<std::is_integral<T>::value>
        initializeParamBinding(MYSQL_BIND& binding, T& value)
        {
            using PureType_t = std::decay_t<T>;
            using Signed_t = std::make_signed_t<PureType_t>;

            binding.buffer = &value;
            binding.buffer_type = detail::toMysqlEnum(getBindingType<Signed_t>());
            binding.is_unsigned = std::is_unsigned<PureType_t>()? true : false;
        }

        template<typename T>
        constexpr inline std::enable_if_t<std::is_floating_point<T>::value>
        initializeParamBinding(MYSQL_BIND& binding, T& value)
        {
            using PureType_t = std::decay_t<T>;
            static_assert(!std::is_same<PureType_t, long double>::value,
                    "'long double' is not supported by mysql prepared statements protocol!");

            binding.buffer = &value;
            binding.buffer_type = detail::toMysqlEnum(getBindingType<PureType_t>());
        }

        template<typename T, typename std::enable_if<CanBindAsParam<BindingTypes::String, T>::value, int>::type=0>
        inline void initializeParamBinding(MYSQL_BIND& binding, T& string)
        {
            binding.buffer = const_cast<void*>(reinterpret_cast<const void*>(string.data()));
            binding.buffer_type = detail::toMysqlEnum(FieldTypes::String);
            binding.buffer_length = string.length();
        }

        template<typename T, typename std::enable_if<CanBindAsParam<BindingTypes::Decimal, T>::value, int>::type=0>
        inline void initializeParamBinding(MYSQL_BIND& binding, T& decimal)
        {
            binding.buffer = decimal.data();
            binding.buffer_type = detail::toMysqlEnum(FieldTypes::NewDecimal);
            binding.buffer_length = decimal.length();
        }

        template<typename T, typename std::enable_if<CanBindAsParam<BindingTypes::Blob, T>::value, int>::type=0>
        inline void initializeParamBinding(MYSQL_BIND& binding, T& blob)
        {
            binding.buffer = blob.data();
            binding.buffer_type = detail::toMysqlEnum(FieldTypes::Blob);
            binding.buffer_length = blob.size();
        }

        template<typename T, typename std::enable_if<CanBindAsParam<BindingTypes::Date, T>::value, int>::type=0>
        inline void initializeParamBinding(MYSQL_BIND& binding, T& date)
        {
            binding.buffer = &date.detail_getBufferRef();
            binding.buffer_type = detail::toMysqlEnum(FieldTypes::Date);
        }

        template<typename T, typename std::enable_if<CanBindAsParam<BindingTypes::Time, T>::value, int>::type=0>
        inline void initializeParamBinding(MYSQL_BIND& binding, T& time)
        {
            binding.buffer = &time.detail_getBufferRef();
            binding.buffer_type = detail::toMysqlEnum(FieldTypes::Time);
        }

        template<typename T, typename std::enable_if<CanBindAsParam<BindingTypes::Datetime, T>::value, int>::type=0>
        inline void initializeParamBinding(MYSQL_BIND& binding, T& dateTime)
        {
            binding.buffer = &dateTime.detail_getBufferRef();
            binding.buffer_type = detail::toMysqlEnum(FieldTypes::Datetime);
        }

        template<typename T, typename std::enable_if<CanBindAsParam<BindingTypes::Timestamp, T>::value, int>::type=0>
        inline void initializeParamBinding(MYSQL_BIND& binding, T& timestamp)
        {
            binding.buffer = &timestamp.detail_getBufferRef();
            binding.buffer_type = detail::toMysqlEnum(FieldTypes::Timestamp);
#ifdef TIMESTAMP_UNSIGNED_BINARY_TYPE
            binding.is_unsigned = true;
#endif
        }

        inline void initializeParamBinding(MYSQL_BIND& binding, const char*& string)
        {
            binding.buffer = const_cast<void*>(reinterpret_cast<const void*>(string));
            binding.buffer_type = detail::toMysqlEnum(FieldTypes::String);
            binding.buffer_length = std::strlen(string);
        }

        template<std::size_t N>
        constexpr inline void initializeParamBinding(MYSQL_BIND& binding, const char (&string)[N])
        {
            binding.buffer = const_cast<void*>(reinterpret_cast<const void*>(string));
            binding.buffer_type = detail::toMysqlEnum(FieldTypes::String);
            static_assert(N>0, "The Universe is falling apart :(");
            binding.buffer_length = N-1;
        }

        template<typename T, typename std::enable_if<CanBindAsParam<BindingTypes::Nullable, T>::value, int>::type=0>
        inline void initializeParamBinding(MYSQL_BIND& binding, T& nullable)
        {
            initializeNullable(binding, nullable);
            if (!*binding.is_null)
            {
                initializeParamBinding(binding, *nullable);
            }
        }


        /*
         * Result bindings
         */
        template<typename T>
        inline std::enable_if_t<std::is_arithmetic<T>::value>
        initializeResultBinding(MYSQL_BIND& binding, T& value)
        {
            initializeParamBinding(binding, value);
        }

        template<typename T, typename std::enable_if<CanBindAsResult<BindingTypes::String, T>::value, int>::type=0>
        inline void initializeResultBinding(MYSQL_BIND& binding, T& string)
        {
            binding.buffer = string.data();
            binding.buffer_type = detail::toMysqlEnum(FieldTypes::String);
            binding.buffer_length = string.maxSize();
            binding.length = &string.counterRef();
        }

        template<typename T, typename std::enable_if<CanBindAsResult<BindingTypes::Decimal, T>::value, int>::type=0>
        inline void initializeResultBinding(MYSQL_BIND& binding, T& decimal)
        {
            binding.buffer = decimal.data();
            binding.buffer_type = detail::toMysqlEnum(FieldTypes::NewDecimal);
            binding.buffer_length = decimal.maxSize();
            binding.length = &decimal.counterRef();
        }

        template<typename T, typename std::enable_if<CanBindAsResult<BindingTypes::Blob, T>::value, int>::type=0>
        inline void initializeResultBinding(MYSQL_BIND& binding, T& blob)
        {
            binding.buffer = blob.data();
            binding.buffer_type = detail::toMysqlEnum(FieldTypes::Blob);
            binding.buffer_length = blob.maxSize();
            binding.length = &blob.counterRef();
        }

        template<typename T, typename std::enable_if<CanBindAsResult<BindingTypes::Date, T>::value, int>::type=0>
        inline void initializeResultBinding(MYSQL_BIND& binding, T& date)
        {
            binding.buffer = &date.detail_getBufferRef();
            binding.buffer_type = detail::toMysqlEnum(FieldTypes::Date);
        }

        template<typename T, typename std::enable_if<CanBindAsResult<BindingTypes::Time, T>::value, int>::type=0>
        inline void initializeResultBinding(MYSQL_BIND& binding, T& time)
        {
            binding.buffer = &time.detail_getBufferRef();
            binding.buffer_type = detail::toMysqlEnum(FieldTypes::Time);
        }

        template<typename T, typename std::enable_if<CanBindAsResult<BindingTypes::Datetime, T>::value, int>::type=0>
        inline void initializeResultBinding(MYSQL_BIND& binding, T& datetime)
        {
            binding.buffer = &datetime.detail_getBufferRef();
            binding.buffer_type = detail::toMysqlEnum(FieldTypes::Datetime);
        }

        template<typename T, typename std::enable_if<CanBindAsResult<BindingTypes::Timestamp, T>::value, int>::type=0>
        inline void initializeResultBinding(MYSQL_BIND& binding, T& timestamp)
        {
            binding.buffer = &timestamp.detail_getBufferRef();
            binding.buffer_type = detail::toMysqlEnum(FieldTypes::Timestamp);
#ifdef TIMESTAMP_UNSIGNED_BINARY_TYPE
            binding.is_unsigned = true;
#endif
        }

        template<typename T, typename std::enable_if<CanBindAsResult<BindingTypes::Nullable, T>::value, int>::type=0>
        inline void initializeResultBinding(MYSQL_BIND& binding, T& nullable)
        {
            initializeNullable(binding, nullable);
            initializeResultBinding(binding, *nullable);
        }


        /*
         * Initialize binding implementation
         */
        template<bool isParamBinding>
        struct InitializeBindingImpl;

        template<>
        struct InitializeBindingImpl<true>
        {
            template<typename Data>
            static void call(MYSQL_BIND& binding, Data& data)
            {
                initializeParamBinding(binding, data);
            }
        };

        template<>
        struct InitializeBindingImpl<false>
        {
            template<typename Data>
            static void call(MYSQL_BIND& binding, Data& data)
            {
                initializeResultBinding(binding, data);
            }
        };


        /*
         * Initialize bindings implementation
         */
        template<bool isParamBinding, int N>
        struct InitializeBindingsImpl
        {
            template<typename Bindings, typename Data>
            static void call(Bindings& bindings, Data& data)
            {
                InitializeBindingImpl<isParamBinding>::call(bindings[N-1], std::get<N-1>(data));
                InitializeBindingsImpl<isParamBinding, N-1>::call(bindings, data);
            }
        };

        template<bool isParamBinding>
        struct InitializeBindingsImpl<isParamBinding, 1>
        {
            template<typename Bindings, typename Data>
            static void call(Bindings& bindings, Data& data)
            {
                InitializeBindingImpl<isParamBinding>::call(bindings[0], std::get<0>(data));
            }
        };

        template<bool isParamBinding>
        struct InitializeBindingsImpl<isParamBinding, 0>
        {
            template<typename Bindings, typename Data>
            static void call(Bindings&, Data&)
            {
            }
        };


        /*
         * Initialize Bindings
         */
        template<bool isParamBinding, typename... Types, typename Bindings, typename Data>
        inline void initializeBindings(Bindings& bindings, Data& data)
        {
            InitializeBindingsImpl<isParamBinding, sizeof...(Types)>::call(bindings, data);
        }
    }
}


