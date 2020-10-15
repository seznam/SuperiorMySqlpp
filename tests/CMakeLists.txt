function(setup_test target)
  target_include_directories(${target}
                             PRIVATE ${CMAKE_SOURCE_DIR}/third_party/bandit)

  if(CMAKE_CXX_COMPILER_ID MATCHES Clang|GNU)
    target_compile_options(
      ${target}
      PRIVATE -pedantic-errors
              -Werror
              -Wall
              -Wextra
              -Wswitch-enum
              -Wnarrowing
              -fsanitize=address
              -fsanitize=leak
              -fsanitize=undefined)
    # TODO: Use target_link_options on CMake V3.13+
    target_link_libraries(
      ${target}
      -fsanitize=address
      -fsanitize=leak
      -fsanitize=undefined
      -Wl,--no-undefined
      -Wl,--warn-common
      -rdynamic)
  endif()

  target_link_libraries(${target} SuperiorMySqlpp)
endfunction()

add_executable(
  test_main
  main.cpp
  traits.cpp
  uncaught_exception_counter.cpp
  converters/converters.cpp
  db_access/connection_pool.cpp
  db_access/connection.cpp
  db_access/driver.cpp
  db_access/dynamic_prepared_statements.cpp
  db_access/master_slave_connection_pools.cpp
  db_access/metadata.cpp
  db_access/prepared_statements.cpp
  db_access/query_escaping.cpp
  db_access/row_stream_adapter.cpp
  db_access/row.cpp
  db_access/simple_result.cpp
  db_access/store_result.cpp
  db_access/transactions.cpp
  types/array.cpp
  types/concat_iterator.cpp
  types/date.cpp
  types/datetime.cpp
  types/decimal_data.cpp
  types/loggers.cpp
  types/nullable.cpp
  types/string_data.cpp
  types/time.cpp)

setup_test(test_main)

add_test(
  NAME test_main
  COMMAND runtest.sh $<TARGET_FILE:test_main>
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if(TEST_ODR_ENABLED OR TEST_EXTENDED_ENABLED)
  find_package(Boost REQUIRED COMPONENTS system)
endif()

if(TEST_ODR_ENABLED)
  add_subdirectory(odr EXCLUDE_FROM_ALL)
endif()

if(TEST_EXTENDED_ENABLED)
  add_subdirectory(extended)
endif()
