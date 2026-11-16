import gc
import struct
import threading
import pytest
import MeTIOT
from conftest import make_wire_packet

# Protocol constants
NODE_HEADER_DEVICE_ID = 0xFE
NODE_HEADER_DATA = 0xFD
DEV_TANK_GUARDIAN = 0xFF
DEV_FILTER_GUARDIAN = 0xFE
TG_TEMPERATURE = 0xFF


def make_id_response(device_type: int, unique_id: int) -> bytes:
    payload = bytes([device_type]) + struct.pack("<Q", unique_id)
    return make_wire_packet(NODE_HEADER_DEVICE_ID, payload)


def test_client_handshake_and_async_callbacks(mock_device):
    received_messages = []
    fatal_errors = []
    msg_event = threading.Event()
    fatal_event = threading.Event()

    def on_data(client, packet_id, payload):
        received_messages.append((packet_id, list(payload)))
        msg_event.set()

    def on_warning(client, code, msg):
        pass

    def on_fatal(client, code, msg):
        fatal_errors.append((code, msg))
        fatal_event.set()

    def device_worker():
        mock_device.accept()
        _ = mock_device.recv_frame()  # DeviceIdentificationRequest (0xFE)
        mock_device.send_raw(make_id_response(DEV_TANK_GUARDIAN, 0x1122334455667788))

        # Send telemetry frame (Temperature = 24.50 C)
        telemetry = bytes([TG_TEMPERATURE]) + struct.pack("<h", 2450)
        mock_device.send_raw(make_wire_packet(NODE_HEADER_DATA, telemetry))

        assert msg_event.wait(timeout=2.0)
        mock_device.close_client()

    t = threading.Thread(target=device_worker)
    t.start()

    client = MeTIOT.DeviceClient("127.0.0.1", mock_device.port)
    client.assign_handlers(on_data, on_warning, on_fatal)
    client.connect()

    assert client.get_unique_id() == 0x1122334455667788
    assert client.get_device_type() == MeTIOT.DeviceType.TANK_GUARDIAN

    client.start_listening()
    assert msg_event.wait(timeout=2.0)

    packet_id, raw_payload = received_messages[0]
    assert packet_id == NODE_HEADER_DATA
    decoded = client.get_protocol_handler().interpret_data(raw_payload)
    assert decoded["Temperature_C"] == pytest.approx(24.50, rel=1e-4)

    # Wait for disconnect fatal callback so background thread exits cleanly
    assert fatal_event.wait(timeout=2.0)
    assert len(fatal_errors) == 1
    assert fatal_errors[0][0] == int(MeTIOT.AsyncErrorCode.SOCKET_FAILURE)

    t.join()
    client.disconnect()


def test_gil_safe_teardown_on_gc_during_disconnect(mock_device):
    """Ensures ~DeviceClient() and ~PythonEventHandler() tear down cleanly on disconnect."""
    fatal_event = threading.Event()
    ready_to_close = threading.Event()

    def device_worker():
        mock_device.accept()
        _ = mock_device.recv_frame()
        mock_device.send_raw(make_id_response(DEV_FILTER_GUARDIAN, 42))
        assert ready_to_close.wait(timeout=2.0)
        mock_device.close_client()

    t = threading.Thread(target=device_worker)
    t.start()

    def run_and_drop_references():
        client = MeTIOT.DeviceClient("127.0.0.1", mock_device.port)
        client.assign_handlers(
            lambda c, pid, data: None,
            None,
            lambda c, code, msg: fatal_event.set(),
        )
        client.connect()
        client.start_listening()
        ready_to_close.set()
        assert fatal_event.wait(timeout=2.0)
        # `client` and handler go out of scope and run destructors here

    run_and_drop_references()
    gc.collect()
    t.join()
