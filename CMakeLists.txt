cmake_minimum_required(VERSION 3.10)
project(MeTIOTPythonLibrary LANGUAGES CXX)

find_package(OpenSSL REQUIRED)

if (NOT OPENSSL_FOUND)
    message(FATAL_ERROR "Could not find OpenSSL libraries. Please install them.")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v3.0.1
)
FetchContent_MakeAvailable(pybind11)

add_subdirectory(src/MeTIOTCommunication)

add_subdirectory(src/python_bindings)