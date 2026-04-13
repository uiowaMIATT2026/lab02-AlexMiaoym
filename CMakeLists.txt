cmake_minimum_required(VERSION 4.2)
project(ece5940_lab02 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

add_subdirectory(src)