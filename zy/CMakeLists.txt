cmake_minimum_required(VERSION 3.0)
project(zy)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

include_directories(.)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

set(LIB_SRC
    zy/log.cc
    zy/util.cc
    zy/config.cc
    )

add_library(zy SHARED ${LIB_SRC})
#add_library(zy_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (zy_static PROPERTIES OUTPUT_NAME "zy")

add_executable(test tests/test.cc)
add_dependencies(test zy)
target_link_libraries(test zy)

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config zy)
target_link_libraries(test_config zy yaml-cpp)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
