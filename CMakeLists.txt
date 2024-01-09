cmake_minimum_required(VERSION 3.27)
project(snake)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})

add_executable(snake src/main.cpp)
target_link_libraries(snake ${SDL2_LIBRARIES} SDL2_ttf)

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
