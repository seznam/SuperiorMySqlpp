/*
 * Author: Tomas Nozicka
 */

#pragma once


#include <array>
#include <cmath>
#include <cstring>


namespace SuperiorMySqlpp
{
    /**
     * Base class for all array based SQL types
     */
    template<std::size_t N>
    class ArrayBase
    {
    protected:
        std::array<char, N> array;

        using ItemsCount_t = unsigned long;
        ItemsCount_t itemsCount = 0;

    public:
        ArrayBase() = default;

        ArrayBase(const ArrayBase& other)
        {
            assign(other);
        }

        ArrayBase(ArrayBase&& other)
        {
            assign(other);
        }

        ArrayBase& operator=(const ArrayBase& other)
        {
            assign(other);
            return *this;
        }

        ArrayBase& operator=(ArrayBase&& other)
        {
            assign(other);
            return *this;
        }

        ~ArrayBase() = default;

    private:
        constexpr ItemsCount_t count() const
        {
            return std::min(itemsCount, static_cast<ItemsCount_t>(N));
        }

        template<std::size_t NN = N>
        std::enable_if_t<(NN > 0),void> assign(const ArrayBase& other)
        {
            std::memcpy(array.data(), other.array.data(), other.size());
            itemsCount = other.size();
        }

        template<std::size_t NN = N>
        std::enable_if_t<(NN == 0),void> assign(const ArrayBase&) {}

    public:
        ItemsCount_t& counterRef()
        {
            return itemsCount;
        }

        const ItemsCount_t& counterRef() const
        {
            return itemsCount;
        }

        auto begin()
        {
            return array.begin();
        }

        auto begin() const
        {
            return array.begin();
        }

        auto cbegin() const
        {
            return array.cbegin();
        }

        auto end()
        {
            return begin() + count();
        }

        auto end() const
        {
            return cbegin() + count();
        }

        auto cend() const
        {
            return cbegin() + count();
        }

        auto endOfStorage()
        {
            return array.end();
        }

        auto endOfStorage() const
        {
            return array.end();
        }

        auto cendOfStorage() const
        {
            return array.cend();
        }

        auto data()
        {
            return array.data();
        }

        constexpr auto data() const
        {
            return array.data();
        }

        auto& front()
        {
            return array.front();
        }

        constexpr const auto& front() const
        {
            return array.front();
        }

        auto& back()
        {
            return array.back();
        }

        constexpr const auto& back() const
        {
            return array.back();
        }

        auto& operator[](std::size_t position)
        {
            return *(begin()+position);
        }

        constexpr const auto& operator[](std::size_t position) const
        {
            return *(begin()+position);
        }

        constexpr auto size() const
        {
            return count();
        }

        constexpr auto maxSize() const
        {
            return array.max_size();
        }

        bool empty() const
        {
            return count() == 0;
        }
    };
}


