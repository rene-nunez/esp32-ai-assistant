"""
Binary protocol v2. [1B type][4B BE len][payload]. 0x01=audio 0x02=text.
Controls (binary MSG_TEXT): VOICE_START / VOICE_END
TTS text is sent as raw WebSocket TEXT frames.

Binary vs JSON: ESP32 has no heap for JSON parsing; bit-shift header = zero alloc.
"""

import struct
from enum import IntEnum

HEADER_SIZE = 5

class MessageType(IntEnum):
    AUDIO = 0x01
    TEXT = 0x02

CMD_VOICE_START = "VOICE_START"
CMD_VOICE_END = "VOICE_END"

def decode(data: bytes) -> tuple[int, bytes]:
    if len(data) < HEADER_SIZE:
        raise ValueError(
            f"Message too short: {len(data)} bytes, need at least {HEADER_SIZE}"
        )
    msg_type = data[0]
    length = struct.unpack(">I", data[1:5])[0]
    if HEADER_SIZE + length > len(data):
        raise ValueError(
            f"Header declares payload of {length} bytes but message has only "
            f"{len(data) - HEADER_SIZE} remaining"
        )
    payload = data[5:5 + length]
    return msg_type, payload