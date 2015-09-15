/*
 * Author: Tomas Nozicka
 */

#pragma once

#include <pthread.h>
#include <cstdlib>
#include <cassert>


/*
 * I'm terribly, terribly sorry about this code (hacks),
 * but the blame is really for those who wrote MySQL C API :(
 * Especially the useless mysql_thread_init and mysql_thread_end using
 * TSS (Thread-specific storage) only for debugging. The first one is automatically
 * called from mysql_init allocating memory for TSS without any notion of setting
 * automatic cleanup! (And yes pthread supports this.)
 */


namespace SuperiorMySqlpp { namespace LowLevel { namespace detail { namespace MySqlInternal
{
    /*
     * libmysqlclient internal structures
     */

    struct AnyPsi;

    struct MySqlCondition
    {
        pthread_cond_t condition;
        AnyPsi* psi;
    };

    struct MySqlMutex
    {
        pthread_mutex_t mutex;
        AnyPsi* psi;
    };

    struct MySqlThreadVariable
    {
        int threadErrno;
        MySqlCondition suspend;
        MySqlMutex mutex;
        volatile MySqlMutex* currentMutex;
        volatile MySqlCondition* currentCond;
        pthread_t pthreadSelf;
        unsigned long id;
        int CompareLength;
        volatile int abort;
        char init;
        struct MySqlThreadVariable *next, **previous;
        void* optionalInfo;
        void* stackEndsHere;
    };
}}}}


/*
 * We need to access internal symbols of libmysqlclient.
 * This is really nasty :( But, again, blame C API authors!
 */
extern "C"
{
    extern SuperiorMySqlpp::LowLevel::detail::MySqlInternal::MySqlMutex THR_LOCK_threads;
    extern pthread_key_t THR_KEY_mysys;
    extern SuperiorMySqlpp::LowLevel::detail::MySqlInternal::MySqlCondition THR_COND_threads;
    extern unsigned int THR_thread_count;
}


namespace SuperiorMySqlpp { namespace LowLevel { namespace detail { namespace MySqlThreadRaii
{
    /*
     * Use this class only with thread_local storage specifier to ensure correct functionality!
     */
    class Cleanup
    {
        MySqlInternal::MySqlThreadVariable* tssPointer;

    public:
        Cleanup()
            : tssPointer(reinterpret_cast<MySqlInternal::MySqlThreadVariable*>(pthread_getspecific(THR_KEY_mysys)))
        {
            assert(tssPointer != nullptr);
        }

        ~Cleanup()
        {
            /*
             * In Debian init is set correctly, but at least on Fedora init is always false.
             * So we cannot use the original condition:
             *
             * if (tssPointer!=nullptr) && tssPointer->init)
             *
             */
            if (tssPointer != nullptr)
            {
                pthread_mutex_destroy(&tssPointer->mutex.mutex);
                free(tssPointer);
                tssPointer = nullptr;

                pthread_mutex_lock(&THR_LOCK_threads.mutex);
                assert(THR_thread_count != 0);
                if (--THR_thread_count == 0)
                {
                    pthread_cond_signal(&THR_COND_threads.condition);
                }
                pthread_mutex_unlock(&THR_LOCK_threads.mutex);
            }

            pthread_setspecific(THR_KEY_mysys, nullptr);
        }
    };

    /*
     * Call after mysql_thread_init to ensure TSS is set and can be captured.
     *
     * tread_local ensures required storage duration and we capture
     * TSS pointer in ctor which could not be found out otherwise in destruction phase
     * since TSS is already destroyed.
     */
    inline void setup()
    {
        thread_local Cleanup cleanup{};
    }
}}}}
