/*
 *  Author: Michal Merc
 */

#pragma once

#include <string>
#include <new>
#include <superior_mysqlpp/prepared_statements/binding_types.hpp>
#include <superior_mysqlpp/types/string_view.hpp>

namespace SuperiorMySqlpp
{

    /*
     *  This data type can be used instead of `StringData` type, allowing you to get string data
     *  without knowing their size in the first place. However, this adds little overhead, so if
     *  you are not dealing with MySQL's `TEXT` types, use `StringData` instead.
     */
    class HugeStringData
    {
    public:
        typedef char value_type;
        typedef value_type *pointer;
        typedef const value_type *const_pointer;
        typedef size_t size_type;
    private:
        /*
         *  readonly shareable container
         */
        struct DataContainer
        {
            size_type size;
            int       numReferences;

        private:
            DataContainer(size_type size) : size(size), numReferences(1) {}

        public:

            static DataContainer *Create(size_t size)
            {
                return new(operator new(sizeof(DataContainer) + size)) DataContainer(size);
            }

            inline pointer data() const
            {
                return const_cast<pointer>(reinterpret_cast<const_pointer>(this + 1));
            }

            inline DataContainer *ref()
            {
                numReferences++;
                return this;
            }

            inline void unref()
            {
                if(--numReferences == 0)
                {
                    operator delete(this);
                }
            }
        };

        DataContainer *container_ { nullptr };
        size_type      targetSize { 0 };

        void refContainer(DataContainer *container)
        {
            if(container != nullptr)
            {
                container_ = container->ref();
            }
        }

        void unrefContainer()
        {
            if(container_ != nullptr)
            {
                container_->unref();
                container_ = nullptr;
            }
        }

    public:
        HugeStringData() = default;

        HugeStringData(const HugeStringData &str) : HugeStringData()
        {
            refContainer(str.container_);
        }

        HugeStringData(HugeStringData &&str) : HugeStringData()
        {
            refContainer(str.container_);
        }

        ~HugeStringData()
        {
            unrefContainer();
        }

        inline HugeStringData &operator =(const HugeStringData &str)
        {
            unrefContainer();
            refContainer(str.container_);
            return *this;
        }

        inline HugeStringData &operator =(HugeStringData &&str)
        {
            unrefContainer();
            refContainer(str.container_);
            return *this;
        }

        inline size_type &counterRef()
        {
            return targetSize;
        }

        inline const size_type &counterRef() const
        {
            return targetSize;
        }

        inline size_type maxSize() const
        {
            return container_ ? container_->size : 0;
        }

        inline pointer data() const
        {
            return container_ ? container_->data() : nullptr;
        }

        inline size_type size() const
        {
            return container_ ? container_->size : 0;
        }

        inline std::string getString() const
        {
            return { data(), size() };
        }

        inline StringView getStringView() const
        {
            return { data(), size() };
        }

        void doAllocate()
        {
            unrefContainer();
            container_ = DataContainer::Create(targetSize);
        }
    };

    namespace detail
    {
        /**
         *  When you are inserting new data you already know its size, so you can use container like std::string
         */
        template<>
        struct CanBindAsParam<BindingTypes::String, HugeStringData> : std::false_type {};

        /**
         *  We need to use special way in our binding system. Just trust me.
         */
        template<>
        struct CanBindAsResult<BindingTypes::String, HugeStringData> : std::false_type {};
    }
}
