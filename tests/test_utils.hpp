#pragma once

#include <new>
#include <utility>

// Class implementing RAII on placement-new allocated data
template <typename T>
class PlacementPtr
{
    T* ptr;

public:
    template <typename... Args>
    PlacementPtr(void* location, Args&&... args)
    {
        ptr = new(location) T(std::forward<Args>(args)...);
    }

    PlacementPtr(void* location)
    {
        // Note that we intentionally don't use T{} or T(),
        // that would force initialization
        ptr = new(location) T;
    }

    ~PlacementPtr()
    {
        ptr->~T();
    }

    T& operator* ()
    {
        return *ptr;
    }

    const T& operator* () const
    {
        return *ptr;
    }
};
