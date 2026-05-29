"""Protocolo binario v2.

Formato de cada mensaje WebSocket:
  [1 byte tipo][4 bytes longitud payload (big-endian)][N bytes payload]

Tipos:
  0x01 = MSG_AUDIO — audio PCM/WAV binario
  0x02 = MSG_TEXT  — texto UTF-8 de control

Mensajes de control (MSG_TEXT):
  Cliente -> Servidor:
    "VOICE_START" — inicio de frase (toggle ON o VAD detectó voz)
    "VOICE_END"   — fin de frase (toggle OFF o silencio sostenido)

  Servidor -> Cliente:
    "PLAY_TEXT:<texto>" — reproducir con audio.connecttospeech()
"""

import struct
from enum import IntEnum

HEADER_SIZE = 5


class MessageType(IntEnum):
    AUDIO = 0x01
    TEXT = 0x02


# Control commands
CMD_VOICE_START = "VOICE_START"
CMD_VOICE_END = "VOICE_END"
CMD_PLAY_TEXT = "PLAY_TEXT:"


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
