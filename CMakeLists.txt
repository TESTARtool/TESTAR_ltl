cmake_minimum_required(VERSION 3.14)
project(spot_checker)

set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_MODULE_PATH "/home/css/usr/lib/;${CMAKE_MODULE_PATH}")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -O2 -std=c++11 -I/usr/local/include -L/usr/local/lib -lboost_system -lboost_filesystem -fopenmp -lpthread -lz")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lstdc++fs -std=c++17")
include_directories(/home/css/spot-2.7.2)   # for spot header files. when installed via config+make
include_directories(/home/css/usr/include)   #for bddx.h file
link_directories(/home/css/usr/lib)


add_executable(JSONwriter JSONwritertest.cpp)
add_executable(spotparse spotparse.cpp)
add_executable(spot_checker spot_checker.cpp)
add_executable(spot_ltlf spot_ltlf.cpp)
target_link_libraries(spot_checker libspot.so libbddx.so libstdc++fs.a) #libstdc++fs.a for filesystem exeprimental
target_link_libraries(spotparse libspot.so)
target_link_libraries(spot_ltlf libspot.so)

