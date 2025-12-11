pybind11_add_module(
    MeTIOT 
    bindings.cpp
)

target_link_libraries(
    MeTIOT 
    PRIVATE 

    # Core C++ Library
    MeTIOTCommunication

    # OpenSSL Library 
    OpenSSL::SSL 
    OpenSSL::Crypto
)

target_include_directories(
    MeTIOT 
    PRIVATE 
    $<TARGET_PROPERTY:MeTIOTCommunication,INTERFACE_INCLUDE_DIRECTORIES>
)