#include "filter_guardian_protocol.hpp"

#include "core/exceptions.hpp"
#include "protocol/protocol_constants.hpp"
#include "utils/byte_conversion.hpp"

std::map<std::string, ProtocolValue> FilterGuardianProtocol::interpret_data(const std::vector<uint8_t>& data) {
    unsigned int currentPlace = 0;
    std::map<std::string, ProtocolValue> organisedData;

    if (data.empty()) {
        throw ProtocolError("Protocol: Data is empty.");
    }

    while (currentPlace < data.size()) {
        uint8_t subheader = data[currentPlace];
        currentPlace += 1; // Move past the subheader

        switch (static_cast<Protocol::DataSubHeader::FilterGuardian>(subheader)) {
        case (Protocol::DataSubHeader::FilterGuardian::FlowRate): {
            // Verify theres enough space in data vector to contain this
            const size_t DATA_SIZE = 2; // Byte count of int16_t
            if (currentPlace + DATA_SIZE > data.size()) {
                throw ProtocolError("Protocol: Not enough space left in data vector to "
                                    "read data defined in data sub header.");
            }

            // Call util to convert 2 byte array to int16_t
            int16_t rawFlowRate = bytes_to_int16(data, currentPlace);

            float flowRateLM = rawFlowRate / 100.0;

            organisedData["Flowrate_LM"] = flowRateLM;

            // Move pointer
            currentPlace += DATA_SIZE;
            break;
        }
        case (Protocol::DataSubHeader::FilterGuardian::Pressure_Port1): {
            // Verify theres enough space in data vector to contain this
            const size_t DATA_SIZE = 2; // Byte count of int16_t
            if (currentPlace + DATA_SIZE > data.size()) {
                throw ProtocolError("Protocol: Not enough space left in data vector to "
                                    "read data defined in data sub header.");
            }

            // Call util to convert 2 byte array to int16_t
            int16_t rawPressure = bytes_to_int16(data, currentPlace);

            float pressurePSI = rawPressure / 1000.0;

            organisedData["Pressure_Port1_PSI"] = pressurePSI;

            // Move pointer
            currentPlace += DATA_SIZE;
            break;
        }
        case (Protocol::DataSubHeader::FilterGuardian::Pressure_Port2): {
            // Verify theres enough space in data vector to contain this
            const size_t DATA_SIZE = 2; // Byte count of int16_t
            if (currentPlace + DATA_SIZE > data.size()) {
                throw ProtocolError("Protocol: Not enough space left in data vector to "
                                    "read data defined in data sub header.");
            }

            // Call util to convert 2 byte array to int16_t
            int16_t rawPressure = bytes_to_int16(data, currentPlace);

            float pressurePSI = rawPressure / 1000.0;

            organisedData["Pressure_Port2_PSI"] = pressurePSI;

            // Move pointer
            currentPlace += DATA_SIZE;
            break;
        }
        case (Protocol::DataSubHeader::FilterGuardian::FloodSensor): {
            // Verify theres enough space in data vector to contain this
            const size_t DATA_SIZE = 1; // Byte count of int16_t
            if (currentPlace + DATA_SIZE > data.size()) {
                throw ProtocolError("Protocol: Not enough space left in data vector to "
                                    "read data defined in data sub header.");
            }

            // Extract data
            const bool flood_sensor_value = data[currentPlace];

            organisedData["Flood_Sensor_Active"] = flood_sensor_value;

            currentPlace += DATA_SIZE;
            break;
        }
        default: {
            throw ProtocolError("Protocol: Unknown data sub header occured.");
        }
        }
    }

    return organisedData;
}
