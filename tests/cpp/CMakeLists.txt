include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)

add_executable(metiot_cpp_tests
    test_protocol.cpp
    test_socket.cpp
    test_utils.cpp
    test_client.cpp
)

target_link_libraries(metiot_cpp_tests
    PRIVATE
    MeTIOTCommunication
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(metiot_cpp_tests)
