/*
 * Author: Tomas Nozicka
 */

#pragma once


#include <string>


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
}
