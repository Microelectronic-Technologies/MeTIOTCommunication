#include "../../include/protocol/fish_tank_protocol.hpp"

std::map<std::string, ProtocolValue> FishTankProtocol::interpret_data(const std::vector<uint8_t>& data) {
    unsigned int currentPlace = 0;
    std::map<std::string, ProtocolValue> organisedData;
    
    if (data.empty()) {
        // TODO: Throw error (Empty data)
    }

    while (currentPlace < data.size()) {

        uint8_t subheader = data[currentPlace];
        currentPlace += 1; // Move past the subheader

        switch(static_cast<Protocol::FishTankDataSubHeader>(subheader)) {
            case (Protocol::FishTankDataSubHeader::Temperature): {
                // Verify theres enough space in data vector to contain this
                const size_t DATA_SIZE = 2; // Byte count of int16_t
                if (currentPlace + DATA_SIZE > data.size()) {
                    // TODO: Throw error (Not enough space left in data vector to contain this data)
                }

                // Call util to convert 2 byte array to int16_t
                int16_t rawTemp = bytes_to_int16(data, currentPlace);

                // TODO any small transformation if required

                organisedData["Temperature_C"] = rawTemp;

                // Move pointer
                currentPlace += DATA_SIZE;
                break;
            }
            case (Protocol::FishTankDataSubHeader::DataEnd): {
                // Data terminator found
                return organisedData;
            }
            default: {
                // TODO: Throw error here (Unknown data)
                return organisedData;
            }
        }
    }
    // TODO: Throw error here (Did not find end of data sub header)
}