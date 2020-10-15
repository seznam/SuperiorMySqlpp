file(GLOB_RECURSE FOUND_HEADERS
     "${CMAKE_CURRENT_SOURCE_DIR}/../../include/*.hpp")
list(TRANSFORM FOUND_HEADERS REPLACE ".*/include/" "")

set(ODR_SOURCES)
foreach(header_file ${FOUND_HEADERS})
  string(REGEX REPLACE "[/\\]" "-" source_file ${header_file})
  get_filename_component(source_file ${source_file} NAME_WE)
  configure_file(odr.tmpl ${source_file}.1.odr.cpp)
  list(APPEND ODR_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${source_file}.1.odr.cpp)
  configure_file(odr.tmpl ${source_file}.2.odr.cpp)
  list(APPEND ODR_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${source_file}.2.odr.cpp)
endforeach()

add_library(test_odr SHARED ${ODR_SOURCES})
setup_test(test_odr)
target_link_libraries(test_odr Boost::system)

add_custom_target(refresh_odr_cmake COMMAND ${CMAKE_COMMAND} -E touch_nocreate ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)
add_dependencies(test_odr refresh_odr_cmake)
