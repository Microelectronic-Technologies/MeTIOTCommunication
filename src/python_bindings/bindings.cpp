#include <pybind11/pybind11.h>
#include <pybind11/stl.h> // Required for std::vector, std::string conversion

#include "client.hpp"
#include "protocol/abstract_protocol.hpp"
#include "protocol/fish_tank_protocol.hpp"

namespace py = pybind11;

using KeyVector = std::vector<uint8_t>;

PYBIND11_MODULE(MeTIOT, m) {
     m.doc() = "Pybind11 bindings for the MeTIOT C++ library.";

     // --- Protocols ---

     // Expose the Abstract Base Class
     py::class_<AbstractProtocol>(m, "AbstractProtocol")
          // NOTE: The C++ constructor is: AbstractProtocol(std::vector<uint8_t>& key)
          // This is ONLY called by derived classes in C++, so we skip exposing it here.
        
          .def("deconstruct_packet", &AbstractProtocol::deconstructPacket, 
               py::arg("packet"), py::arg("data"), 
               "Decodes and validates a packet, storing the payload in 'data'.")
        
          .def("create_rejection_packet", &AbstractProtocol::createRejectionPacket,
               py::arg("packet"),
               "Creates a rejection packet in case of an invalid message.")
          ;

     // Expose a Concrete Derived Class
     py::class_<FishTankProtocol, AbstractProtocol>(m, "FishTankProtocol")
          // Constructor: FishTankProtocol(std::vector<uint8_t>& key)
          // Python users must pass a bytes-like object (e.g., bytes([1, 2, 3...])) 
          // which pybind11 converts to std::vector<uint8_t>.
          .def(py::init<KeyVector&>(), py::arg("key"),
               "Initializes the protocol handler with a symmetric encryption key.")
          ;
    

     // --- DeviceClient ---

     py::class_<DeviceClient>(m, "DeviceClient")
          // Constructor: takes IP (string) and Port (int)
          .def(py::init<const std::string&, int>(),
               py::arg("ip"), py::arg("port"),
               "Initializes the device client with IP and port.")
        
          // Standard methods
          .def("connect", &DeviceClient::connect, "Connects to the remote device.")
          .def("disconnect", &DeviceClient::disconnect, "Disconnects from the remote device.")
          .def("send_packet", &DeviceClient::send_packet, 
               py::arg("packet"), 
               "Sends a raw data packet (bytes) to the device.")

          // Exposing the Protocol Handler pointer
          .def("get_protocol_handler", &DeviceClient::get_protocol_handler, 
             // This policy is CRITICAL. It tells Pybind11:
             // 1. Don't try to delete the returned object (it's owned by unique_ptr in DeviceClient).
             // 2. Keep the DeviceClient instance alive as long as the returned protocol object is referenced in Python.
             py::return_value_policy::reference_internal, 
             "Returns the active protocol handler object.")
          ;
}