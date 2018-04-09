/*
 *  Author: Michal Merc
 */

#pragma once

#include <superior_mysqlpp/utils.hpp>
#include <superior_mysqlpp/types/nullable.hpp>
#include <type_traits>
#include <string>
#include <vector>

namespace SuperiorMySqlpp
{
    namespace detail
    {
        template<typename T>
        struct CanBeDynamic : public std::false_type {};

        template<typename T>
        struct CanBeDynamic<std::basic_string<T>> : public std::true_type {};

        template<typename T>
        struct CanBeDynamic<std::vector<T>> : public std::true_type {};

        template<typename T>
        struct CanBeDynamic<T &> : public CanBeDynamic<T> {};

        template<typename T>
        struct CanBeDynamic<Nullable<T>> : public CanBeDynamic<T> {};

        /**
         * Data are filled in driver and so, we don't have access to its underlying type.
         * -> We are forced to use virtual functions (or good old C function pointers in this case)
         */
        class DynamicStorageBase
        {
        public:
            typedef void *(*DynamicAllocFunc)(void *, unsigned long &);

            size_t targetSize;
            DynamicAllocFunc dynamicAlloc;
            void *targetBuffer;

            constexpr DynamicStorageBase(DynamicAllocFunc func = nullptr, void *targetBuffer = nullptr) 
                : targetSize(0), dynamicAlloc(func), targetBuffer(targetBuffer) {}

            constexpr DynamicStorageBase(const DynamicStorageBase &b)
                : targetSize(b.targetSize), dynamicAlloc(b.dynamicAlloc), targetBuffer(b.targetBuffer) {}
        };
    }

    /**
     * Dynamic type container
     *   this type will allow you to load arbitrary large strings or blob types.
     *   Currently, there are two supported types: std::basic_string<> and std::vector<>.
     *   However, there is no support for charset conversions, so ensure you are
     *   using correct underlying type.
     *
     * You can combine Dynamic with Nullable (e.g. Nullable<Dynamic<std::string>>)
     */
    template<typename T, bool = detail::CanBeDynamic<T>::value>
    class Dynamic;

    template<typename T>
    class Dynamic<T, true> : public T
    {
    public:
        class DynamicStorageImpl : public detail::DynamicStorageBase
        {
        public:
            constexpr DynamicStorageImpl() : DynamicStorageBase(&dynamicAllocImpl) {}

            static void *dynamicAllocImpl(void *object, unsigned long &dstLength) {
                auto self = static_cast<DynamicStorageImpl *>(object);
                Dynamic<T, true> *target = containerOf(self, &Dynamic<T, true>::impl_);
                target->resize((dstLength = self->targetSize) / sizeof(typename T::value_type));
                return reinterpret_cast<void *>(&*target->begin());
            }
        };

        DynamicStorageImpl impl_;

        using T::T;
    };

    template<typename T>
    class Dynamic<T &, true> : public detail::DynamicStorageBase
    {
    public:
        inline Dynamic(T &target) : DynamicStorageBase(&dynamicAllocImpl, static_cast<void *>(&target)) {}

        static void *dynamicAllocImpl(void *object, unsigned long &dstLength) {
            auto self = static_cast<detail::DynamicStorageBase *>(object);
            T &target = *static_cast<T *>(self->targetBuffer);
            target.resize((dstLength = self->targetSize) / sizeof(typename T::value_type));
            return reinterpret_cast<void *>(&*target.begin());
        }
    };

    template<typename T>
    class Dynamic<T, false> {
    public:
        static_assert(detail::CanBeDynamic<T>::value, "T is not dynamic type");
    };

    namespace detail {

        /**
         * Transforms types to dynamic, if necessary
         */
        template<typename T, bool = CanBeDynamic<T>::value>
        struct DynamicTypeTransform {
            typedef T type;
        };

        template<typename T>
        struct DynamicTypeTransform<T, true> {
            typedef Dynamic<T> type;
        };

        template<typename T>
        struct DynamicTypeTransform<Nullable<T>, true> {
            typedef Nullable<Dynamic<T>> type;
        };

    }
}

