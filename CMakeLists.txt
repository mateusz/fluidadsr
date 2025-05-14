cmake_minimum_required(VERSION 3.10)
project(fluidadsr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FLUIDSYNTH REQUIRED fluidsynth)

add_executable(fluidadsr
    src/main.cpp)

target_include_directories(fluidadsr PRIVATE ${FLUIDSYNTH_INCLUDE_DIRS})
target_link_libraries(fluidadsr PRIVATE ${FLUIDSYNTH_LIBRARIES})

install(TARGETS fluidadsr DESTINATION bin)