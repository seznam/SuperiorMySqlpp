#pragma once
#include <superior_mysqlpp/row.hpp>


namespace SuperiorMySqlpp
{
    /**
     * Adapter for Row that provides operator>>() for data extraction.
     * Example:
     *
     *  auto row = ...
     *  std::string s;
     *  int i = 0;
     *  SuperiorMySqlpp::RowStream {row}
     *      >> s
     *      >> i
     *      ;
     */
    class RowStream
    {
    private:
        Row::Iterator first;
        Row::Iterator last;

    public:
        explicit RowStream(Row& row)
            : first {row.begin()}
            , last {row.end()}
        {
        }

        template<typename T>
        RowStream& operator>>(T& obj)
        {
            assert (first != last);
            if (!(*first).isNull()) {
                obj = (*first).to<T>();
            }
            ++first;
            return *this;
        }
    };
}
