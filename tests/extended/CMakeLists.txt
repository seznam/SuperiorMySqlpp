add_executable(test_extended dnsa_connection_pool.cpp ../main.cpp)
target_link_libraries(test_extended Boost::system)
setup_test(test_extended)
target_compile_options(test_extended PRIVATE -Wno-deprecated-declarations)

# TODO Remove sudo requirement
add_test(
  NAME test_extended
  COMMAND sudo ${CMAKE_CURRENT_SOURCE_DIR}/../runtest.sh
          $<TARGET_FILE:test_extended>
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..)
