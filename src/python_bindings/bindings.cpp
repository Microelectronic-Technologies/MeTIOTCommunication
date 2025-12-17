#include <pybind11/pybind11.h>
#include <pybind11/stl.h> 

#include "client.hpp"
#include "PythonEventHandler.hpp"
#include "protocol/abstract_protocol.hpp"
#include "protocol/fish_tank_protocol.hpp"
#include "protocol/protocol_constants.hpp"

namespace py = pybind11;

using KeyVector = std::vector<uint8_t>;

// Forward declaration of the C++ function that handles the dynamic cast
py::object cast_protocol_handler(DeviceClient& client);

// ----------------- For converting python data map <str:protocolValue> to dict
py::object protocol_value_to_py_object(const ProtocolValue& value) {
     return std::visit([](auto&& arg) -> py::object {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float>) {
               return py::cast(arg);
          } else {
               return py::cast(static_cast<int>(arg));
          }
     }, value);
}

std::map<std::string, py::object> wrap_interpret_data(AbstractProtocol& self, const std::vector<uint8_t>& data) {
     // Call pure cpp function
     std::map<std::string, ProtocolValue> cppMap = self.interpret_data(data);

     // Translate pure cpp into python map
     std::map<std::string, py::object> pythonMap;
     for (const auto& [key, value] : cppMap) {
          pythonMap[key] = protocol_value_to_py_object(value);
     }
     return pythonMap;
}
// ----------------- For converting python data map <str:protocolValue> to dict

PYBIND11_MODULE(MeTIOT, m) {
     m.doc() = "Pybind11 bindings for the MeTIOT C++ library.";

     // --- Exceptions ---
     static py::exception<LibraryError> lib_exc(m, "LibraryError", PyExc_RuntimeError);
     static py::exception<SocketError> sock_exc(m, "SocketError", lib_exc.ptr());
     static py::exception<ProtocolError> prot_exc(m, "ProtocolError", lib_exc.ptr());
     static py::exception<EncodingError> enc_exc(m, "EncodingError", lib_exc.ptr());

     py::register_exception_translator([](std::exception_ptr p) {
          try {
               if (p) std::rethrow_exception(p);
          } catch (const SocketError &e) {
               py::set_error(sock_exc, e.what());
          } catch (const ProtocolError &e) {
               py::set_error(prot_exc, e.what());
          } catch (const EncodingError &e) {
               py::set_error(enc_exc, e.what());
          } catch (const LibraryError &e) {
               py::set_error(lib_exc, e.what());
          }
     });

     // --- Enums --- 
     py::enum_<DeviceType>(m, "DeviceType")
        .value("UNKNOWN", DeviceType::UNKNOWN)
        .value("FISH_TANK", DeviceType::FISH_TANK)
        .export_values(); // access via DeviceType.UNKNOWN

     // Incoming headers
     py::module_ inc = m.def_submodule("Incoming", "Protocol Headers");

     py::enum_<Protocol::IncomingHeader::General>(inc, "General")
          .value("MalformedPacket", Protocol::IncomingHeader::General::MalformedPacketNotification)
          // Exclude DeviceIdentifier as it's not used in python
          .value("Data", Protocol::IncomingHeader::General::Data)
          .export_values();

     py::enum_<Protocol::IncomingHeader::FishTank>(inc, "FishTank")
          .export_values();


     // --- Protocols ---

     // Expose the Abstract Base Class
     py::class_<AbstractProtocol>(m, "AbstractProtocol")
          // Base class methods
          .def("deconstruct_packet", 
               (std::pair<uint8_t, std::vector<uint8_t>> (AbstractProtocol::*)(const std::vector<uint8_t>&))
               &AbstractProtocol::deconstruct_packet, 
               py::arg("packet"), 
               "Decodes and validates a packet, returning a tuple (status, data).")
          .def("create_rejection_packet", &AbstractProtocol::create_rejection_packet,
               "Creates a rejection packet in case of an invalid message.")
          .def("interpret_data", 
               &wrap_interpret_data,
               py::arg("data"),
               "Interprets the raw payload data into a director of typed values (str:x)")
          ;

     // Expose a Concrete Derived Class (FishTankProtocol)
     py::class_<FishTankProtocol, AbstractProtocol>(m, "FishTankProtocol")
          // Constructor
          .def(py::init<>(),
               "Initializes the protocol handler with a symmetric encryption key.")
          ;
    

     // --- DeviceClient ---

     py::class_<DeviceClient>(m, "DeviceClient")
          // Constructor
          .def(py::init<const std::string&, int>(),
               py::arg("ip"), py::arg("port"),
               "Initializes the device client with IP and port.")
        
          // Standard methods
          .def("connect", &DeviceClient::connect, "Connects to the remote device.")
          .def("disconnect", &DeviceClient::disconnect, "Disconnects from the remote device.")
          .def("send_packet", &DeviceClient::send_packet, 
               py::arg("packet"), 
               "Sends a raw data packet (bytes) to the device.")

          // ** FIX 1: Bind the getter needed by the cast_protocol_handler function **
          .def("get_protocol_handler", &DeviceClient::get_protocol_handler, 
               py::return_value_policy::reference_internal,
               "Internal method to retrieve the raw AbstractProtocol pointer.")

          .def("get_device_type", &DeviceClient::get_device_type, "Returns the identified device type.")

          // 2. The CRITICAL Binding Point - This calls the casting function
          .def("get_specific_protocol_handler", 
             [](DeviceClient& client) { return cast_protocol_handler(client); },
             py::return_value_policy::reference_internal,
             "Returns the protocol handler cast to its specific derived type (e.g., FishTankProtocol).")
          .def("assign_receive_handler", [](DeviceClient &self, py::function func) {
               auto wrapper = std::make_shared<PythonEventHandler>(std::move(func));

               self.assign_receive_handler(wrapper);
          }, py::arg("handler_func"), "Assigns a python function to handle incoming messages.")
          .def("start_listening", &DeviceClient::start_listening, "Starts the background network thread.")
          .def("get_device_type", &DeviceClient::get_device_type, "Returns the type of the device.")
             ;
}

// *** Implementation of the Casting Helper Function (No changes needed here) ***

py::object cast_protocol_handler(DeviceClient& client) {
    AbstractProtocol* base_ptr = client.get_protocol_handler();
    if (!base_ptr) {
        throw std::runtime_error("Protocol handler is not initialized.");
    }
    
    DeviceType type = client.get_device_type();
    
    // Check the device type and perform the safe dynamic_cast
    if (type == DeviceType::FISH_TANK) {
        // dynamic_cast is safe because the C++ logic ensures the underlying object is a FishTankProtocol
        FishTankProtocol* derived_ptr = dynamic_cast<FishTankProtocol*>(base_ptr);
        
        if (derived_ptr == nullptr) {
            // This case indicates a logic error in the C++ code (e.g., mismatch between deviceType and actual object)
            throw std::runtime_error("Internal error: Handler type mismatch. Expected FishTankProtocol.");
        }
        
        // py::cast uses the C++ type information to expose the correct Python class
        return py::cast(derived_ptr, py::return_value_policy::reference_internal);
    }
    
    // Default case: return the base class pointer, only exposing base methods.
    return py::cast(base_ptr, py::return_value_policy::reference_internal);
}