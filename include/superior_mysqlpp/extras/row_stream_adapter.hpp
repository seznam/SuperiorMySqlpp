#pragma once

#include <superior_mysqlpp/row.hpp>


namespace SuperiorMySqlpp { namespace Extras
{
    /**
     * Adapter for Row that provides operator>>() for data extraction.
     * Example:
     *
     *  auto row = ...
     *  std::string s;
     *  int i = 0;
     *  SuperiorMySqlpp::Extras::RowStreamAdapter {row}
     *      >> s
     *      >> i
     *      ;
     */
    class RowStreamAdapter
    {
    private:
        Row::Iterator first;
        Row::Iterator last;

    public:
        explicit RowStreamAdapter(Row& row)
            : first {row.begin()}
            , last {row.end()}
        {
        }

        /** Returns true iff it is safe to extract value. */
        explicit operator bool() const
        {
            return first != last;
        }

        /**
         * Extracts a value from row by means of assigning it to \p obj and
         * advancing an internal pointer.
         *
         * If the value is SQL NULL, default-constructed value is assigned.
         * If it is not safe to extract value, behaviour is undefined.
         */
        template<typename T>
        RowStreamAdapter& operator>>(T& obj)
        {
            if (!(*first).isNull()) {
                obj = (*first).to<T>();
            } else {
                obj = T{};
            }
            ++first;
            return *this;
        }
    };
}}
