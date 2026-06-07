#include "config.h"

using namespace websockets;

void send_protocol(uint8_t type, const uint8_t* payload, size_t len) { // wire: [1B type][4B big-endian len][payload]
    uint8_t header[5];                                             // big-endian matches Python struct ">I"
    header[0] = type;                                              // no JSON — zero alloc to parse
    header[1] = (len >> 24) & 0xFF;
    header[2] = (len >> 16) & 0xFF;
    header[3] = (len >> 8)  & 0xFF;
    header[4] = len & 0xFF;

    size_t total = 5 + len;
    uint8_t* buf = (uint8_t*)malloc(total);                        // malloc/free per msg (sporadic) avoids permanent 1 kB buffer
    if (!buf) return;
    memcpy(buf, header, 5);
    memcpy(buf + 5, payload, len);
    ws_client.sendBinary((const char*)buf, total);
    free(buf);
}

void send_control(const char* command) {
    send_protocol(MSG_TEXT, (const uint8_t*)command, strlen(command));
}
