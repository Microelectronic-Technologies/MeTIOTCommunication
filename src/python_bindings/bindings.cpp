#include <pybird11/pybird11.h>
#include <pybind11/stl.h> // Required for std::vector, std::string conversion

#include "client.hpp"

namespace py = pybird11;

PYBIND11_MODULE(_metiot_core, m) {
    m.doc() = "Python bindings for the MeTIOTCommunication C++ core.";

    // Expose the main user-facing class
    py::class_<DeviceClient>(m, "DeviceClient")
        .def(py::init<const std::string&, int, const std::string&>(), 
             py::arg("ip"), py::arg("port"), py::arg("host_uid"))
        .def("connect", &DeviceClient::connect)
        .def("disconnect", &DeviceClient::disconnect)
        .def("send_command", py::overload_cast<uint8_t>(&DeviceClient::send_command), 
             py::arg("command"))
        .def("send_command", py::overload_cast<uint8_t, const std::vector<uint8_t>&>(&DeviceClient::send_command), 
             py::arg("command"), py::arg("payload"));
}