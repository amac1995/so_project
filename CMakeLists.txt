cmake_minimum_required(VERSION 3.14)
project(so_project C)

set(GCC_COVERAGE_COMPILE_FLAGS "-Wall -Wextra -Wconversion -pthread ")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS} /home/alex/CLionProjects/so_project)
link_directories(${GTK3_LIBRARY_DIRS} /home/alex/CLionProjects/so_project)

add_definitions(${GTK3_CFLAGS_OTHER})

add_executable(so_project_server main.c my_protocol.c my_protocol.h jsmn.c jsmn.h)
add_executable(so_project_client client.c)
target_link_libraries(so_project_client ${GTK3_LIBRARIES} )
