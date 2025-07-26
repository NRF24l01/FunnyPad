cmake_minimum_required(VERSION 3.16)
project(FunnyPad LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PULSE REQUIRED libpulse-simple)

add_subdirectory(src/ui)
add_subdirectory(src/audio)

add_executable(funnypad
    src/main.cpp
    resources.qrc
)

target_link_libraries(funnypad
    Qt6::Widgets
    ui
    ${PULSE_LIBRARIES}
)

target_include_directories(funnypad PRIVATE ${PULSE_INCLUDE_DIRS})
