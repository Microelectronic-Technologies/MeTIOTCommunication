#pragma once

#include <stdexcept>
#include <string>

class LibraryError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Socket Errors
class SocketError : public LibraryError {
    using LibraryError::LibraryError;
};

// Protocol Errors
class ProtocolError : public LibraryError {
    using LibraryError::LibraryError;
};

// Encoding/COBS Errors
class EncodingError : public LibraryError {
    using LibraryError::LibraryError;
};

class LogicError : public LibraryError {
    using LibraryError::LibraryError;
};

// --- Asynchronous Error & Warning Codes ---
enum class AsyncErrorCode : int {
    TIMEOUT_OR_INACTIVITY = 100, // No data recv in correct amount of time
    SOCKET_ERROR          = 101, // SocketError
    LIBRARY_ERROR         = 200, // Occurs after warnings threshold passed, LibraryError (EncodingError or ProtocolError)
    UNEXPECTED_SYSTEM_ERR = 999, // std::exception caught
};