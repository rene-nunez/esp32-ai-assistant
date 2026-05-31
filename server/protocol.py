"""Binary protocol v2.

Each WebSocket message format:
  [1 byte type][4 bytes payload length (big-endian)][N bytes payload]

Types:
  0x01 = MSG_AUDIO — PCM/WAV audio binary
  0x02 = MSG_TEXT  — UTF-8 control text

Control messages (MSG_TEXT):
  Client -> Server:
    "VOICE_START" — start of speech (toggle ON or VAD detected voice)
    "VOICE_END"   — end of speech (toggle OFF or silence timeout)

  Server -> Client:
    "PLAY_TEXT:<text>" — play with audio.connecttospeech()
    "PLAY_URL:<url>"   — play Orpheus WAV served via HTTP
"""

import struct
from enum import IntEnum

HEADER_SIZE = 5


class MessageType(IntEnum):
    AUDIO = 0x01
    TEXT = 0x02


CMD_VOICE_START = "VOICE_START"
CMD_VOICE_END = "VOICE_END"
CMD_PLAY_TEXT = "PLAY_TEXT:"
CMD_PLAY_URL = "PLAY_URL:"


def encode(msg_type: int, payload: bytes) -> bytes:
    length = len(payload)
    header = struct.pack(">BI", msg_type & 0xFF, length)
    return header + payload


def decode(data: bytes) -> tuple[int, bytes]:
    msg_type = data[0]
    length = struct.unpack(">I", data[1:5])[0]
    payload = data[5:5 + length]
    return msg_type, payload


def encode_text(text: str) -> bytes:
    return encode(MessageType.TEXT, text.encode("utf-8"))


def encode_audio(pcm_or_wav: bytes) -> bytes:
    return encode(MessageType.AUDIO, pcm_or_wav)
