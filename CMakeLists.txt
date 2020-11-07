# this makefile is intended to be used  on a full Ubuntu 18.04 VM and CLION IDE.
# with SPOT pre-installed.
cmake_minimum_required(VERSION 3.10)
project(spot_checker)

set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_MODULE_PATH "/home/css/usr/lib/;${CMAKE_MODULE_PATH}")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -O2 -std=c++11 -I/usr/local/include -L/usr/local/lib -lboost_system -lboost_filesystem -fopenmp -lpthread -lz")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lstdc++fs -std=c++17")
include_directories(/home/css/spot-2.7.2)   # for spot header files. when installed via config+make
include_directories(/home/css/usr/include)   #for bddx.h file
link_directories(/home/css/usr/lib)

add_executable(spot_checker spot_checker.cpp )
target_link_libraries(spot_checker  libspot.so libbddx.so libstdc++fs.a) #libstdc++fs.a for filesystem exeprimental

#add_executable(spot_formula_to_dot spot_formula_to_dot.cpp)
#target_link_libraries( spot_formula_to_dot libspot.so libbddx.so libstdc++fs.a) #libstdc++fs.a for filesystem exeprimental

