cmake_minimum_required(VERSION 3.0.0)

# Set the default CMAKE_BUILD_TYPE to Release.
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif ()

project(clingo C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

set(SOURCES
    src/config.c
    src/main.c
)

add_executable(clingo ${SOURCES})
target_include_directories(clingo
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(clingo
    ${GTK3_LIBRARIES}
    X11
)

install(TARGETS clingo DESTINATION bin)
