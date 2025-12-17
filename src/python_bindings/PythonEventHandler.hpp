#include "../MeTIOTCommunication/include/interfaces/event_handler.hpp"
#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>

namespace py = pybind11;

class PythonEventHandler : public IEventHandler {
public:
    // Stores the user's Python function (must be py::function/py::object)
    PythonEventHandler(py::function callback) : callback_(std::move(callback)) {}

    // Implementation of the pure virtual C++ method
    void handle_message(int packet_id, const std::vector<uint8_t>& payload) override {
        // --- GIL ACQUISITION MAGIC ---
        // This is the C++ network thread, so we MUST acquire the GIL
        py::gil_scoped_acquire acquire;
        
        try {
            // Call the Python function with C++ types that pybind11 converts
            callback_(packet_id, payload);
            
        } catch (const py::error_already_set &e) {
            // Handle Python exceptions gracefully
            PyErr_Print(); // Print the Python traceback
            std::cerr << "Exception in Python callback: " << e.what() << std::endl;
        }
    }

private:
    py::function callback_; // Stores the Python function object
};