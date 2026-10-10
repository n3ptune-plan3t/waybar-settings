cmake_minimum_required(VERSION 3.16)
project(waybar-settings LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(waybar-settings
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/layoutpage.cpp
    src/layoutpage.h
    src/stylepage.cpp
    src/stylepage.h
    src/preview.cpp
    src/preview.h
    src/widgets.cpp
    src/widgets.h
    src/schema.cpp
    src/schema.h
    src/style.cpp
    src/style.h
    src/jsonc.cpp
    src/jsonc.h
)

target_link_libraries(waybar-settings PRIVATE Qt6::Widgets)
target_compile_options(waybar-settings PRIVATE -Wall -Wextra)

install(TARGETS waybar-settings RUNTIME DESTINATION bin)
