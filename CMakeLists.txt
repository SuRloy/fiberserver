cmake_minimum_required(VERSION 3.0)
project(zy)


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(.)
include_directories(./zy)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

# 定义参与编译的源文件
# aux_source_directory(./zy BASE_LIST)
# aux_source_directory(./zy/utils UTIL_LIST)
# set(LIB_SRC ${BASE_LIST} ${UTIL_LIST})
set(LIB_SRC
    zy/log.cc
    zy/utils/util.cc
    zy/thread.cc
    zy/fiber.cc
    zy/scheduler.cc
    )

set(LIBS
    zy
    pthread
    )

add_executable(test_thread "tests/test_thread.cc" ${LIB_SRC})
target_link_libraries(test_thread ${LIBS})

add_executable(test_fiber "tests/test_fiber.cc" ${LIB_SRC})
target_link_libraries(test_fiber ${LIBS})

add_library(zy SHARED ${LIB_SRC})

