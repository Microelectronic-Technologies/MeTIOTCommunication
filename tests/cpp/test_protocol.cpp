#include "../../src/MeTIOTCommunication/src/core/exceptions.hpp"
#include "../../src/MeTIOTCommunication/src/protocol/abstract_protocol.hpp"
#include "../../src/MeTIOTCommunication/src/protocol/device_agnostic_protocol.hpp"
#include "../../src/MeTIOTCommunication/src/protocol/filter_guardian_protocol.hpp"
#include "../../src/MeTIOTCommunication/src/protocol/protocol_constants.hpp"
#include "../../src/MeTIOTCommunication/src/protocol/tank_guardian_protocol.hpp"
#include "../../src/MeTIOTCommunication/src/utils/byte_conversion.hpp"
#include "../../src/MeTIOTCommunication/src/utils/cobs.hpp"
#include "../../src/MeTIOTCommunication/src/utils/crc.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <cstdint>
#include <cstring>
#include <variant>
#include <vector>

using Catch::Matchers::Equals;
using Catch::Matchers::WithinRel;

namespace {

// Minimal concrete subclass to test AbstractProtocol directly if it has pure virtual methods
class TestableProtocol : public AbstractProtocol {
  public:
    using AbstractProtocol::construct_packet; // Exposes protected method for unit testing

    std::map<std::string, ProtocolValue> interpret_data(const std::vector<uint8_t>&) override { return {}; }
};

void append_int16(std::vector<uint8_t>& buf, int16_t val) {
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
}

} // namespace

// ============================================================================
// 1. AbstractProtocol (COBS framing + Little-Endian CRC)
// ============================================================================

TEST_CASE("AbstractProtocol construct and deconstruct round-trip", "[protocol][abstract]") {
    TestableProtocol proto;

    SECTION("Header + multi-byte payload with zero bytes round-trips cleanly") {
        const uint8_t header = 0x12;
        const std::vector<uint8_t> payload = {0x00, 0xAA, 0x00, 0xFF, 0x55};

        std::vector<uint8_t> raw = {header};
        raw.insert(raw.end(), payload.begin(), payload.end());

        std::vector<uint8_t> encoded = proto.construct_packet(raw);
        REQUIRE_FALSE(encoded.empty());

        auto [decodedHeader, decodedData] = proto.deconstruct_packet(encoded);
        CHECK(decodedHeader == header);
        CHECK_THAT(decodedData, Equals(payload));
    }

    SECTION("Header-only packet (empty payload) round-trips cleanly") {
        const uint8_t header = 0x01;
        std::vector<uint8_t> encoded = proto.construct_packet({header});

        auto [decodedHeader, decodedData] = proto.deconstruct_packet(encoded);
        CHECK(decodedHeader == header);
        CHECK(decodedData.empty());
    }

    SECTION("create_rejection_packet encodes MalformedPacketNotification") {
        std::vector<uint8_t> encoded = proto.create_rejection_packet();
        auto [header, data] = proto.deconstruct_packet(encoded);

        CHECK(header == static_cast<uint8_t>(Protocol::ControllerHeader::General::MalformedPacketNotification));
        CHECK(data.empty());
    }
}

TEST_CASE("AbstractProtocol deconstruct_packet rejects invalid inputs", "[protocol][abstract][error]") {
    TestableProtocol proto;

    SECTION("Empty packet throws ProtocolError") {
        CHECK_THROWS_AS(proto.deconstruct_packet({}), ProtocolError);
    }

    SECTION("Packet shorter than CRC_SIZE after COBS decode throws ProtocolError") {
        // 1 byte payload -> COBS encoded -> decodes to 1 byte (< CRC_SIZE of 2)
        std::vector<uint8_t> tooShort = cobs_encode({0xAA});
        CHECK_THROWS_AS(proto.deconstruct_packet(tooShort), ProtocolError);
    }

    SECTION("Corrupted payload byte throws EncodingError on CRC mismatch") {
        std::vector<uint8_t> raw = {0x10, 0x20, 0x30, 0x40};
        std::vector<uint8_t> encoded = proto.construct_packet(raw);

        // Flip a bit in the payload region (avoiding COBS overhead byte at index 0)
        encoded.back() ^= 0x01;

        CHECK_THROWS_AS(proto.deconstruct_packet(encoded), EncodingError);
    }

    SECTION("Corrupted CRC byte throws EncodingError") {
        std::vector<uint8_t> raw = {0x10, 0x20, 0x30};
        std::vector<uint8_t> encoded = proto.construct_packet(raw);

        // Index 1 is the first byte after COBS code byte (lowByte of CRC)
        encoded[1] ^= 0xFF;

        CHECK_THROWS_AS(proto.deconstruct_packet(encoded), EncodingError);
    }
}

// ============================================================================
// 2. DeviceAgnosticProtocol
// ============================================================================

TEST_CASE("DeviceAgnosticProtocol builds requests and extracts device details", "[protocol][agnostic]") {
    DeviceAgnosticProtocol proto;

    SECTION("create_device_id_request produces valid packet with DeviceIdentificationRequest header") {
        std::vector<uint8_t> packet = proto.create_device_id_request();
        auto [header, data] = proto.deconstruct_packet(packet);

        CHECK(header == static_cast<uint8_t>(Protocol::ControllerHeader::General::DeviceIdentificationRequest));
        CHECK(data.empty());
    }

    SECTION("extract_device_details parses 1-byte deviceType and 8-byte deviceID") {
        const uint8_t expectedType = 0x03;
        const uint64_t expectedID = 0x0102030405060708ULL;

        std::vector<uint8_t> payload(9);
        payload[0] = expectedType;
        std::memcpy(&payload[1], &expectedID, sizeof(uint64_t));

        auto [deviceType, deviceID] = proto.extract_device_details(payload);
        CHECK(deviceType == expectedType);
        CHECK(deviceID == expectedID);
    }

    SECTION("extract_device_details throws ProtocolError when payload size != 9") {
        CHECK_THROWS_AS(proto.extract_device_details({}), ProtocolError);
        CHECK_THROWS_AS(proto.extract_device_details(std::vector<uint8_t>(8, 0x00)), ProtocolError);
        CHECK_THROWS_AS(proto.extract_device_details(std::vector<uint8_t>(10, 0x00)), ProtocolError);
    }
}

// ============================================================================
// 3. FilterGuardianProtocol
// ============================================================================

TEST_CASE("FilterGuardianProtocol interprets valid sensor frames", "[protocol][filter_guardian]") {
    FilterGuardianProtocol proto;
    using Sub = Protocol::DataSubHeader::FilterGuardian;

    SECTION("Single FlowRate reading scales int16 by 1/100") {
        std::vector<uint8_t> data = {static_cast<uint8_t>(Sub::FlowRate)};
        append_int16(data, 1234); // Expect 12.34 L/m

        auto result = proto.interpret_data(data);
        REQUIRE(result.count("Flowrate_LM") == 1);
        CHECK_THAT(std::get<float>(result["Flowrate_LM"]), WithinRel(12.34f, 1e-4f));
    }

    SECTION("Negative FlowRate and Pressure values decode properly") {
        std::vector<uint8_t> data;
        data.push_back(static_cast<uint8_t>(Sub::FlowRate));
        append_int16(data, -250); // -2.50 L/m
        data.push_back(static_cast<uint8_t>(Sub::Pressure_Port1));
        append_int16(data, -1500); // -1.500 PSI

        auto result = proto.interpret_data(data);
        CHECK_THAT(std::get<float>(result["Flowrate_LM"]), WithinRel(-2.50f, 1e-4f));
        CHECK_THAT(std::get<float>(result["Pressure_Port1_PSI"]), WithinRel(-1.500f, 1e-4f));
    }

    SECTION("FloodSensor handles not-fitted (0xFF), dry (0x00), and wet (0x01) states") {
        // Not fitted (0xFF)
        auto resNotFitted = proto.interpret_data({static_cast<uint8_t>(Sub::FloodSensor), 0xFF});
        CHECK(std::get<int>(resNotFitted["Flood_Sensor_Fitted"]) == 0);
        CHECK(std::get<int>(resNotFitted["Flood_Sensor_Active"]) == 0);

        // Fitted & inactive (0x00)
        auto resDry = proto.interpret_data({static_cast<uint8_t>(Sub::FloodSensor), 0x00});
        CHECK(std::get<int>(resDry["Flood_Sensor_Fitted"]) == 1);
        CHECK(std::get<int>(resDry["Flood_Sensor_Active"]) == 0);

        // Fitted & active (0x01)
        auto resWet = proto.interpret_data({static_cast<uint8_t>(Sub::FloodSensor), 0x01});
        CHECK(std::get<int>(resWet["Flood_Sensor_Fitted"]) == 1);
        CHECK(std::get<int>(resWet["Flood_Sensor_Active"]) == 1);
    }

    SECTION("Combined multi-sensor packet parses all fields sequentially") {
        std::vector<uint8_t> data;
        data.push_back(static_cast<uint8_t>(Sub::FlowRate));
        append_int16(data, 500); // 5.00 L/m
        data.push_back(static_cast<uint8_t>(Sub::Pressure_Port1));
        append_int16(data, 30000); // 30.000 PSI
        data.push_back(static_cast<uint8_t>(Sub::Pressure_Port2));
        append_int16(data, 28500); // 28.500 PSI
        data.push_back(static_cast<uint8_t>(Sub::FloodSensor));
        data.push_back(0x01);

        auto result = proto.interpret_data(data);
        REQUIRE(result.size() == 5);
        CHECK_THAT(std::get<float>(result["Flowrate_LM"]), WithinRel(5.00f, 1e-4f));
        CHECK_THAT(std::get<float>(result["Pressure_Port1_PSI"]), WithinRel(30.000f, 1e-4f));
        CHECK_THAT(std::get<float>(result["Pressure_Port2_PSI"]), WithinRel(28.500f, 1e-4f));
        CHECK(std::get<int>(result["Flood_Sensor_Fitted"]) == 1);
        CHECK(std::get<int>(result["Flood_Sensor_Active"]) == 1);
    }
}

TEST_CASE("FilterGuardianProtocol rejects empty, truncated, and unknown subheaders",
          "[protocol][filter_guardian][error]") {
    FilterGuardianProtocol proto;
    using Sub = Protocol::DataSubHeader::FilterGuardian;

    SECTION("Empty data throws ProtocolError") {
        CHECK_THROWS_AS(proto.interpret_data({}), ProtocolError);
    }

    SECTION("Truncated FlowRate payload throws ProtocolError") {
        // Subheader present, only 1 of 2 required bytes provided
        CHECK_THROWS_AS(proto.interpret_data({static_cast<uint8_t>(Sub::FlowRate), 0x01}), ProtocolError);
        // Subheader present, 0 payload bytes provided
        CHECK_THROWS_AS(proto.interpret_data({static_cast<uint8_t>(Sub::Pressure_Port1)}), ProtocolError);
        CHECK_THROWS_AS(proto.interpret_data({static_cast<uint8_t>(Sub::Pressure_Port2)}), ProtocolError);
        CHECK_THROWS_AS(proto.interpret_data({static_cast<uint8_t>(Sub::FloodSensor)}), ProtocolError);
    }

    SECTION("Unknown subheader throws ProtocolError") {
        CHECK_THROWS_AS(proto.interpret_data({0xAA, 0x00, 0x00}), ProtocolError);
    }
}

// ============================================================================
// 4. TankGuardianProtocol
// ============================================================================

TEST_CASE("TankGuardianProtocol interprets temperature and validates bounds", "[protocol][tank_guardian]") {
    TankGuardianProtocol proto;
    using Sub = Protocol::DataSubHeader::TankGuardian;

    SECTION("Valid Temperature reading scales int16 by 1/100") {
        std::vector<uint8_t> data = {static_cast<uint8_t>(Sub::Temperature)};
        append_int16(data, 2345); // 23.45 C

        auto result = proto.interpret_data(data);
        REQUIRE(result.count("Temperature_C") == 1);
        CHECK_THAT(std::get<float>(result["Temperature_C"]), WithinRel(23.45f, 1e-4f));
    }

    SECTION("Sub-zero temperature decodes accurately") {
        std::vector<uint8_t> data = {static_cast<uint8_t>(Sub::Temperature)};
        append_int16(data, -450); // -4.50 C

        auto result = proto.interpret_data(data);
        CHECK_THAT(std::get<float>(result["Temperature_C"]), WithinRel(-4.50f, 1e-4f));
    }

    SECTION("Empty, truncated, or unknown subheader throws ProtocolError") {
        CHECK_THROWS_AS(proto.interpret_data({}), ProtocolError);
        CHECK_THROWS_AS(proto.interpret_data({static_cast<uint8_t>(Sub::Temperature), 0x01}), ProtocolError);
        CHECK_THROWS_AS(proto.interpret_data({0xFD, 0x00, 0x00}), ProtocolError);
    }
}
