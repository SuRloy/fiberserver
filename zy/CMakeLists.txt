cmake_minimum_required(VERSION 3.0)
project(zy)

include(cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(.)
include_directories(/usr/local/include)
include_directories(/home/zouyang/app/local/include)#实验室服务器地址
link_directories(/usr/local/lib)
link_directories(/home/zouyang/app/local/lib)#实验室服务器地址

set(LIB_SRC
    zy/log.cc
    zy/util.cc
    zy/config.cc
    )

add_library(zy SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(zy) #__FILE__
#add_library(zy_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (zy_static PROPERTIES OUTPUT_NAME "zy")

add_executable(test tests/test.cc)
add_dependencies(test zy)
force_redefine_file_macro_for_sources(test) #__FILE__
target_link_libraries(test zy yaml-cpp)

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config zy)
force_redefine_file_macro_for_sources(test_config) #__FILE__
target_link_libraries(test_config zy yaml-cpp)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
