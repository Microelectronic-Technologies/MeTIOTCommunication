import socket
import struct
import threading
import pytest


def calculate_crc(data: bytes) -> int:
    crc = 0
    for byte in data:
        a = ((crc >> 8) ^ byte) & 0xFFFF
        crc = ((a << 2) ^ (a << 1) ^ a ^ (crc << 8)) & 0xFFFF
    return crc


def cobs_encode(data: bytes) -> bytes:
    if not data:
        return b"\x01\x00"
    out = bytearray([0])
    code_idx = 0
    code = 1
    for b in data:
        if b != 0:
            out.append(b)
            code += 1
        if b == 0 or code == 0xFF:
            out[code_idx] = code
            code = 1
            code_idx = len(out)
            out.append(0)
    out[code_idx] = code
    out.append(0x00)
    return bytes(out)


def make_wire_packet(header: int, payload: bytes = b"") -> bytes:
    raw = bytes([header]) + payload
    crc = calculate_crc(raw)
    framed = struct.pack("<H", crc) + raw
    return cobs_encode(framed)


class MockIoTDevice:
    def __init__(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind(("127.0.0.1", 0))
        self.sock.listen(1)
        self.sock.settimeout(2.0)
        self.port = self.sock.getsockname()[1]
        self.conn = None

    def accept(self):
        self.conn, _ = self.sock.accept()
        self.conn.settimeout(2.0)

    def recv_frame(self) -> bytes:
        buf = bytearray()
        while True:
            b = self.conn.recv(1)
            if not b:
                break
            buf.extend(b)
            if b == b"\x00":
                break
        return bytes(buf)

    def send_raw(self, data: bytes):
        self.conn.sendall(data)

    def close_client(self):
        if self.conn:
            try:
                self.conn.close()
            except OSError:
                pass
            self.conn = None

    def close(self):
        self.close_client()
        self.sock.close()


@pytest.fixture
def mock_device():
    device = MockIoTDevice()
    yield device
    device.close()
