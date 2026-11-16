cmake_minimum_required(VERSION 3.10)
project(MeTIOTPythonLibrary LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_PYTHON_BINDINGS "Build Python pybind11 module" ON)
option(BUILD_TESTING "Build C++ unit tests" OFF)

add_subdirectory(src/MeTIOTCommunication)

if(BUILD_PYTHON_BINDINGS)
    include(FetchContent)
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG        v3.1.0
        FIND_PACKAGE_ARGS CONFIG
    )
    FetchContent_MakeAvailable(pybind11)
    add_subdirectory(src/python_bindings)
endif()

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests/cpp)
endif()
