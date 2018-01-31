/*
 * Author: Tomas Nozicka
 */

#pragma once

#include <mysql/mysql.h>

#include <cinttypes>

#include <superior_mysqlpp/exceptions.hpp>
#include <superior_mysqlpp/field_types.hpp>


namespace SuperiorMySqlpp
{
    namespace detail
    {
        /**
         * Mapping of native C++ types to fitting MySQL field type.
         * @return FieldType enumeration of matching type.
         * Specializations for various matching arithmetic types exist.
         */
        template<typename T>
        inline constexpr FieldTypes getBindingType() = delete;

        /**
         * @remark Specialization for int8_t, see generic version.
         */
        template<>
        inline constexpr FieldTypes getBindingType<int8_t>()
        {
            return FieldTypes::Tiny;
        }

        /**
         * @remark Specialization for int16_t, see generic version.
         */
        template<>
        inline constexpr FieldTypes getBindingType<int16_t>()
        {
            return FieldTypes::Short;
        }

        /**
         * @remark Specialization for int32_t, see generic version.
         */
        template<>
        inline constexpr FieldTypes getBindingType<int32_t>()
        {
            return FieldTypes::Long;
        }

        /**
         * @remark Specialization for int64_t, see generic version.
         */
        template<>
        inline constexpr FieldTypes getBindingType<int64_t>()
        {
            return FieldTypes::LongLong;
        }

        /**
         * @remark Specialization for float, see generic version.
         */
        template<>
        inline constexpr FieldTypes getBindingType<float>()
        {
            return FieldTypes::Float;
        }

        /**
         * @remark Specialization for double, see generic version.
         */
        template<>
        inline constexpr FieldTypes getBindingType<double>()
        {
            return FieldTypes::Double;
        }

        /**
         * Returns name of given field type.
         * @param fieldType Enumeration of type #FieldTypes.
         * @return C string of stringified field type.
         */
        inline const char* getBindingTypeName(FieldTypes fieldType)
        {
            switch (fieldType)
            {
                case FieldTypes::Tiny:
                    return "Tiny";

                case FieldTypes::Short:
                    return "Short";

                case FieldTypes::Int24:
                    return "Int24";

                case FieldTypes::Long:
                    return "Long";

                case FieldTypes::LongLong:
                    return "LongLong";

                case FieldTypes::Float:
                    return "Float";

                case FieldTypes::Double:
                    return "Double";

                case FieldTypes::Decimal:
                    return "Decimal";

                case FieldTypes::NewDecimal:
                    return "NewDecimal";

                case FieldTypes::Time:
                    return "Time";

                case FieldTypes::Date:
                    return "Date";

                case FieldTypes::Datetime:
                    return "Datetime";

                case FieldTypes::Timestamp:
                    return "Timestamp";

                case FieldTypes::Year:
                    return "Year";

                case FieldTypes::String:
                    return "String";

                case FieldTypes::VarString:
                    return "VarString";

                case FieldTypes::Enum:
                    return "Enum";

                case FieldTypes::Set:
                    return "Set";

                case FieldTypes::Bit:
                    return "Bit";

                case FieldTypes::Blob:
                    return "Blob";

                case FieldTypes::TinyBlob:
                    return "TinyBlob";

                case FieldTypes::MediumBlob:
                    return "MediumBlob";

                case FieldTypes::LongBlob:
                    return "LongBlob";

                case FieldTypes::Geometry:
                    return "Geometry";

                case FieldTypes::Null:
                    return "Null";

                default:
                    throw std::logic_error{"The Universe is falling apart!"};
            }
        }

        /**
         * Returns name of given field type including the signedness.
         * @return C string of stringified field type.
         */
        inline const char* getBindingTypeFullName(FieldTypes fieldType, bool isUnsigned)
        {
            switch (fieldType)
            {
                case FieldTypes::Tiny:
                    return isUnsigned? "Unsigned Tiny" : "Signed Tiny";

                case FieldTypes::Short:
                    return isUnsigned? "Unsigned Short" : "Signed Short";

                case FieldTypes::Int24:
                    return isUnsigned? "Unsigned Int24" : "Signed Int24";

                case FieldTypes::Long:
                    return isUnsigned? "Unsigned Long" : "Signed Long";

                case FieldTypes::LongLong:
                    return isUnsigned? "Unsigned LongLong" : "Signed LongLong";

                case FieldTypes::Float:
                case FieldTypes::Double:
                case FieldTypes::Decimal:
                case FieldTypes::NewDecimal:
                case FieldTypes::Time:
                case FieldTypes::Date:
                case FieldTypes::Datetime:
                case FieldTypes::Timestamp:
                case FieldTypes::Year:
                case FieldTypes::String:
                case FieldTypes::VarString:
                case FieldTypes::Enum:
                case FieldTypes::Set:
                case FieldTypes::Bit:
                case FieldTypes::Blob:
                case FieldTypes::TinyBlob:
                case FieldTypes::MediumBlob:
                case FieldTypes::LongBlob:
                case FieldTypes::Geometry:
                case FieldTypes::Null:
                default:
                    return getBindingTypeName(fieldType);
            }
        }
    }
}
