cmake_minimum_required(VERSION 3.10)
project(MeTIOTCommunication LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enforce strict warnings for better error checking
add_compile_options(-Wall -Wextra -Wpedantic)

file(GLOB_RECURSE SOURCE_FILES 
    "src/*.cpp"
)

add_library(
    MeTIOTCommunication
    STATIC
    ${SOURCE_FILES}
)

target_include_directories(
    MeTIOTCommunication
    PUBLIC
        # Set the 'include' directory as the primary search path for headers
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)