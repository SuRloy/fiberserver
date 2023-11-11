cmake_minimum_required(VERSION 3.0)
project(ss)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

set(LIB_SRC
    ss/log.cc
    )

add_library(ss SHARED ${LIB_SRC})
#add_library(ss_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (ss_static PROPERTIES OUTPUT_NAME "ss")

add_executable(test tests/test.cc)
add_dependencies(test ss)
target_link_libraries(test ss)


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
