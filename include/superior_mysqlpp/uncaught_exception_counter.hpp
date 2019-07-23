/*
 * Author: Tomas Nozicka
 */

#pragma once

#include <cxxabi.h>

namespace SuperiorMySqlpp
{
    inline int uncaughtExceptions() noexcept
    {
    // There is no `__cxa_get_clobals` in newer libc++ abi, so we are forced to use
    // `__cxa_uncaught_exceptions`
#if defined(_LIBCPPABI_VERSION) && _LIBCPPABI_VERSION >= 1002
        return static_cast<int>(__cxxabiv1::__cxa_uncaught_exceptions());
#else
        auto* globalsPtr = __cxxabiv1::__cxa_get_globals();
        auto* globalsEreasedPtr = reinterpret_cast<char*>(globalsPtr);
        auto* uncaughtExceptionsErasedPtr = globalsEreasedPtr + sizeof(void*);
        auto* uncaughtExceptionsPtr = reinterpret_cast<int*>(uncaughtExceptionsErasedPtr);
        return *uncaughtExceptionsPtr;
#endif
    }

    class UncaughtExceptionCounter
    {
    private:
        int exceptionCount;

    public:
        UncaughtExceptionCounter() noexcept
            : exceptionCount{uncaughtExceptions()}
        {}

        bool isNewUncaughtException() const noexcept
        {
            return uncaughtExceptions() > exceptionCount;
        }
    };
}
