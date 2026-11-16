import struct
import pytest
import MeTIOT

# Protocol::DataSubHeader::FilterGuardian
FG_FLOW_RATE = 0xFF
FG_PRESSURE_PORT1 = 0xFE
FG_PRESSURE_PORT2 = 0xFD
FG_FLOOD_SENSOR = 0xFC

# Protocol::DataSubHeader::TankGuardian
TG_TEMPERATURE = 0xFF


def test_filter_guardian_interpret_data_types():
    proto = MeTIOT.FilterGuardianProtocol()

    payload = (
        bytes([FG_FLOW_RATE])
        + struct.pack("<h", 1234)
        + bytes([FG_PRESSURE_PORT1])
        + struct.pack("<h", 30000)
        + bytes([FG_PRESSURE_PORT2])
        + struct.pack("<h", 28500)
        + bytes([FG_FLOOD_SENSOR, 0x01])
    )

    result = proto.interpret_data(list(payload))

    assert isinstance(result, dict)
    assert result["Flowrate_LM"] == pytest.approx(12.34, rel=1e-4)
    assert result["Pressure_Port1_PSI"] == pytest.approx(30.000, rel=1e-4)
    assert result["Pressure_Port2_PSI"] == pytest.approx(28.500, rel=1e-4)
    assert result["Flood_Sensor_Fitted"] == 1
    assert isinstance(result["Flood_Sensor_Fitted"], int)
    assert result["Flood_Sensor_Active"] == 1


def test_tank_guardian_interpret_data():
    proto = MeTIOT.TankGuardianProtocol()
    payload = bytes([TG_TEMPERATURE]) + struct.pack("<h", -450)  # -4.50 C

    result = proto.interpret_data(list(payload))
    assert result["Temperature_C"] == pytest.approx(-4.50, rel=1e-4)


def test_cpp_exceptions_translate_to_python():
    proto = MeTIOT.TankGuardianProtocol()

    with pytest.raises(MeTIOT.ProtocolError):
        proto.interpret_data([])

    with pytest.raises(MeTIOT.LibraryError):
        proto.interpret_data([0xAA, 0x00, 0x00])
