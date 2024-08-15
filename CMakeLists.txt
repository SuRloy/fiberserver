cmake_minimum_required(VERSION 3.0)
project(zy)

include(cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(.)
include_directories(/home/zouyang/app/local/include)#实验室服务器地址
include_directories(/usr/local/include)
link_directories(/home/zouyang/app/local/lib)#实验室服务器地址
link_directories(/usr/local/lib)


set(LIB_SRC
    zy/log.cc
    zy/util.cc
    zy/config.cc
    zy/scheduler.cc
    zy/thread.cc
    zy/fiber.cc
    zy/timer.cc
    zy/iomanager.cc
    )

add_library(zy SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(zy) #__FILE__
#add_library(zy_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (zy_static PROPERTIES OUTPUT_NAME "zy")

set(LIB_LIB
    zy
    pthread
    yaml-cpp
    )

add_executable(test tests/test.cc)
add_dependencies(test zy)
force_redefine_file_macro_for_sources(test) #__FILE__
target_link_libraries(test ${LIB_LIB})

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config zy)
force_redefine_file_macro_for_sources(test_config) #__FILE__
target_link_libraries(test_config ${LIB_LIB})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread zy)
force_redefine_file_macro_for_sources(test_thread) #__FILE__
target_link_libraries(test_thread ${LIB_LIB})

add_executable(test_util tests/test_util.cc)
add_dependencies(test_util zy)
force_redefine_file_macro_for_sources(test_util) #__FILE__
target_link_libraries(test_util ${LIB_LIB})

add_executable(test_fiber tests/test_fiber.cc)
add_dependencies(test_fiber zy)
force_redefine_file_macro_for_sources(test_fiber) #__FILE__
target_link_libraries(test_fiber ${LIB_LIB})

add_executable(test_scheduler tests/test_scheduler.cc)
add_dependencies(test_scheduler zy)
force_redefine_file_macro_for_sources(test_scheduler) #__FILE__
target_link_libraries(test_scheduler ${LIB_LIB})

add_executable(test_iomanager tests/test_iomanager.cc)
add_dependencies(test_iomanager zy)
force_redefine_file_macro_for_sources(test_iomanager) #__FILE__
target_link_libraries(test_iomanager ${LIB_LIB})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
