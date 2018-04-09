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

    template<class P, class M>
    size_t offsetOf(const M P::*member)
    {
        return (size_t) &(reinterpret_cast<P *>(0)->*member);
    }

    template<class P, class M>
    P *containerOf(M *ptr, const M P::*member)
    {
        return (P *)((char*)ptr - offsetOf(member));
    }
}
