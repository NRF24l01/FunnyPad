cmake_minimum_required(VERSION 3.16)
project(Qt6Hello LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(hello
    main.cpp
)

target_link_libraries(hello
    Qt6::Widgets
)
