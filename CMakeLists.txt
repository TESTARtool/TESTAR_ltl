cmake_minimum_required(VERSION 3.14)
project(spot_checker)

set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_MODULE_PATH "/home/css/usr/lib/;${CMAKE_MODULE_PATH}")

include_directories(/home/css/spot-2.7.2)   # for spot header files. when installed via config+make
include_directories(/home/css/usr/include)   #for bddx.h file
link_directories(/home/css/usr/lib)


add_executable(spotparse spotparse.cpp)
add_executable(spot_checker spot_checker.cpp)
target_link_libraries(spot_checker libspot.so libbddx.so)
target_link_libraries(spotparse libspot.so)

