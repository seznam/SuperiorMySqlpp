set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

if (NOT CMAKE_USE_PTHREADS_INIT)
    message(FATAL_ERROR "SuperiorMySql++ requires pthread!")
endif()
