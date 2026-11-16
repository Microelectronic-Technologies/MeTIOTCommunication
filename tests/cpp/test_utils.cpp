#include "../../src/MeTIOTCommunication/src/core/exceptions.hpp"
#include "../../src/MeTIOTCommunication/src/utils/byte_conversion.hpp"
#include "../../src/MeTIOTCommunication/src/utils/cobs.hpp"
#include "../../src/MeTIOTCommunication/src/utils/crc.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

using Catch::Matchers::Equals;
using Catch::Matchers::WithinRel;

// ============================================================================
// 1. Byte Conversion (Little-Endian)
// ============================================================================

TEST_CASE("byte_conversion decodes 8-bit signed and unsigned integers", "[utils][bytes]") {
    // Prefix padding byte (0xAA) at index 0 to test non-zero currentPlace offsets
    const std::vector<uint8_t> buf = {0xAA, 0x00, 0x7F, 0x80, 0xFF};

    CHECK(bytes_to_uint8(buf, 1) == 0);
    CHECK(bytes_to_uint8(buf, 2) == 127);
    CHECK(bytes_to_uint8(buf, 3) == 128);
    CHECK(bytes_to_uint8(buf, 4) == 255);

    CHECK(bytes_to_int8(buf, 1) == 0);
    CHECK(bytes_to_int8(buf, 2) == 127);
    CHECK(bytes_to_int8(buf, 3) == -128);
    CHECK(bytes_to_int8(buf, 4) == -1);
}

TEST_CASE("byte_conversion decodes 16-bit little-endian integers", "[utils][bytes]") {
    // Index 1..2: 0x04D2 (1234) in LE -> {0xD2, 0x04}
    // Index 3..4: 0x8000 (-32768 / 32768) in LE -> {0x00, 0x80}
    // Index 5..6: 0xFFFF (-1 / 65535) in LE -> {0xFF, 0xFF}
    const std::vector<uint8_t> buf = {0xEE, 0xD2, 0x04, 0x00, 0x80, 0xFF, 0xFF};

    CHECK(bytes_to_int16(buf, 1) == 1234);
    CHECK(bytes_to_uint16(buf, 1) == 1234u);

    CHECK(bytes_to_int16(buf, 3) == std::numeric_limits<int16_t>::min());
    CHECK(bytes_to_uint16(buf, 3) == 32768u);

    CHECK(bytes_to_int16(buf, 5) == -1);
    CHECK(bytes_to_uint16(buf, 5) == std::numeric_limits<uint16_t>::max());
}

TEST_CASE("byte_conversion decodes 32-bit little-endian integers", "[utils][bytes]") {
    // 0x12345678 in LE -> {0x78, 0x56, 0x34, 0x12}
    // -1 / 0xFFFFFFFF in LE -> {0xFF, 0xFF, 0xFF, 0xFF}
    const std::vector<uint8_t> buf = {0x99, 0x78, 0x56, 0x34, 0x12, 0xFF, 0xFF, 0xFF, 0xFF};

    CHECK(bytes_to_int32(buf, 1) == 0x12345678);
    CHECK(bytes_to_uint32(buf, 1) == 0x12345678u);

    CHECK(bytes_to_int32(buf, 5) == -1);
    CHECK(bytes_to_uint32(buf, 5) == std::numeric_limits<uint32_t>::max());
}

TEST_CASE("byte_conversion decodes 64-bit little-endian integers", "[utils][bytes]") {
    // 0x0102030405060708 in LE -> {0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01}
    const std::vector<uint8_t> buf = {
        0xCC, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80 // INT64_MIN / 2^63
    };

    CHECK(bytes_to_int64(buf, 1) == 0x0102030405060708LL);
    CHECK(bytes_to_uint64(buf, 1) == 0x0102030405060708ULL);

    CHECK(bytes_to_int64(buf, 9) == std::numeric_limits<int64_t>::min());
    CHECK(bytes_to_uint64(buf, 9) == 0x8000000000000000ULL);
}

TEST_CASE("byte_conversion decodes IEEE-754 float and double values", "[utils][bytes]") {
    SECTION("float round-trip with offset") {
        const float expected = -123.456f;
        std::vector<uint8_t> buf(2 + sizeof(float), 0xAA);
        std::memcpy(&buf[2], &expected, sizeof(float));

        CHECK_THAT(bytes_to_float(buf, 2), WithinRel(expected, 1e-6f));
    }

    SECTION("double round-trip with offset") {
        const double expected = 3.141592653589793;
        std::vector<uint8_t> buf(3 + sizeof(double), 0x55);
        std::memcpy(&buf[3], &expected, sizeof(double));

        CHECK_THAT(bytes_to_double(buf, 3), WithinRel(expected, 1e-12));
    }

    SECTION("Special IEEE-754 values (0.0 and -infinity)") {
        const float negInf = -std::numeric_limits<float>::infinity();
        std::vector<uint8_t> buf(sizeof(float));
        std::memcpy(buf.data(), &negInf, sizeof(float));

        CHECK(std::isinf(bytes_to_float(buf, 0)));
        CHECK(bytes_to_float(buf, 0) < 0.0f);
    }
}

// ============================================================================
// 2. COBS Encoding & Decoding
// ============================================================================

TEST_CASE("COBS encodes and decodes payloads cleanly", "[utils][cobs]") {
    SECTION("Encoded output contains no interior zero bytes and ends with 0x00 delimiter") {
        const std::vector<uint8_t> input = {0x00, 0x11, 0x00, 0x00, 0x22, 0x33, 0x00};
        const std::vector<uint8_t> encoded = cobs_encode(input);

        REQUIRE(encoded.size() >= 2);
        CHECK(encoded.back() == 0x00);
        // No 0x00 anywhere before the final delimiter
        CHECK(std::find(encoded.begin(), encoded.end() - 1, 0x00) == encoded.end() - 1);

        const std::vector<uint8_t> decoded = cobs_decode(encoded);
        CHECK_THAT(decoded, Equals(input));
    }

    SECTION("Known COBS test vectors match specification") {
        // {0x00} -> {0x01, 0x01, 0x00}
        CHECK_THAT(cobs_encode({0x00}), Equals(std::vector<uint8_t>{0x01, 0x01, 0x00}));
        CHECK_THAT(cobs_decode({0x01, 0x01, 0x00}), Equals(std::vector<uint8_t>{0x00}));

        // {0x11, 0x22, 0x00, 0x33} -> {0x03, 0x11, 0x22, 0x02, 0x33, 0x00}
        const std::vector<uint8_t> raw = {0x11, 0x22, 0x00, 0x33};
        const std::vector<uint8_t> wire = {0x03, 0x11, 0x22, 0x02, 0x33, 0x00};
        CHECK_THAT(cobs_encode(raw), Equals(wire));
        CHECK_THAT(cobs_decode(wire), Equals(raw));
    }

    SECTION("254-byte maximum block boundary (code == 0xFF) does not inject extra zero") {
        for (size_t len : {253u, 254u, 255u, 510u}) {
            std::vector<uint8_t> nonZeroPayload(len, 0x5A);
            std::vector<uint8_t> encoded = cobs_encode(nonZeroPayload);

            CHECK(encoded.back() == 0x00);
            CHECK(std::find(encoded.begin(), encoded.end() - 1, 0x00) == encoded.end() - 1);

            std::vector<uint8_t> decoded = cobs_decode(encoded);
            CHECK_THAT(decoded, Equals(nonZeroPayload));
        }
    }

    SECTION("Empty input handling") {
        // cobs_decode({}) returns empty vector
        CHECK(cobs_decode({}).empty());

        // Minimal framed empty payload {0x01, 0x00} decodes to empty vector
        CHECK(cobs_decode({0x01, 0x00}).empty());
    }
}

TEST_CASE("COBS decoder throws EncodingError on malformed frames", "[utils][cobs][error]") {
    SECTION("Single byte buffer without delimiter throws EncodingError") {
        CHECK_THROWS_AS(cobs_decode({0x01}), EncodingError);
        CHECK_THROWS_AS(cobs_decode({0x00}), EncodingError);
    }

    SECTION("Buffer missing trailing 0x00 delimiter throws EncodingError") {
        CHECK_THROWS_AS(cobs_decode({0x03, 0x11, 0x22}), EncodingError);
    }

    SECTION("Early 0x00 delimiter inside payload throws EncodingError") {
        // Code says 3 bytes follow, but 0x00 appears immediately
        CHECK_THROWS_AS(cobs_decode({0x03, 0x00, 0x22, 0x00}), EncodingError);
    }

    SECTION("Block size exceeding remaining buffer throws EncodingError") {
        // Code 0x05 claims 4 data bytes follow, but only 2 exist before trailing 0x00
        CHECK_THROWS_AS(cobs_decode({0x05, 0x11, 0x22, 0x00}), EncodingError);
    }
}

// ============================================================================
// 3. CRC Calculation & Verification
// ============================================================================

TEST_CASE("CRC calculation and verification behave consistently", "[utils][crc]") {
    SECTION("Empty input produces deterministic CRC of 0x0000") {
        const uint16_t crc = calculate_crc({});
        CHECK(crc == 0x0000);
        CHECK(check_crc(crc, {}));
        CHECK_FALSE(check_crc(0x0001, {}));
    }

    SECTION("check_crc validates matching data and rejects modified data") {
        const std::vector<uint8_t> payload = {0x01, 0x02, 0x03, 0x04, 0xAA, 0x55};
        const uint16_t validCrc = calculate_crc(payload);

        CHECK(check_crc(validCrc, payload));
        CHECK_FALSE(check_crc(static_cast<uint16_t>(validCrc ^ 0x0001), payload));

        // Single-bit flip in any byte of payload fails CRC check
        for (size_t i = 0; i < payload.size(); ++i) {
            std::vector<uint8_t> tampered = payload;
            tampered[i] ^= 0x01;
            CHECK_FALSE(check_crc(validCrc, tampered));
        }
    }

    SECTION("CRC is sensitive to byte order and trailing zeros") {
        const uint16_t crcAB = calculate_crc({0x12, 0x34});
        const uint16_t crcBA = calculate_crc({0x34, 0x12});
        const uint16_t crcAB0 = calculate_crc({0x12, 0x34, 0x00});

        CHECK(crcAB != crcBA);
        CHECK(crcAB != crcAB0);
    }
}
