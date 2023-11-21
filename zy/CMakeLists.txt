cmake_minimum_required(VERSION 3.0)
project(zy)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

set(LIB_SRC
    zy/log.cc
    zy/util.cc
    )

add_library(zy SHARED ${LIB_SRC})
#add_library(zy_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (zy_static PROPERTIES OUTPUT_NAME "zy")

add_executable(test tests/test.cc)
add_dependencies(test zy)
target_link_libraries(test zy)


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
