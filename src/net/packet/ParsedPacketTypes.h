#pragma once

/*
 *         Common Packet Structure
 * ┌──────────────────────────────────────┐
 * │ CommonPacketHeader (8 bytes)         │
 * │ ┌────────┬────────────┬────────────┐ │
 * │ │ ver(1) │ protoid(1) │ bodyLen(2) │ │
 * │ └────────┴────────┴────────────────┘ │
 * │  flags / reserved (4)                │
 * ├──────────────────────────────────────┤
 * │ Body (N bytes = bodyLen)             │
 * │ ┌────────────────────────────────┐   │
 * │ │ protocol-specific payload (28) │   │
 * │ └────────────────────────────────┘   │
 * └──────────────────────────────────────┘
 */

/*
 *            LOGIN REQ BODY
 *
 *  Login request packet body layout.
 *  Strings are encoded as:
 *  [uint16 length] + [raw bytes (no null-termination)]
 *
 * ┌──────────────────────────────────────────┐
 * │ idLen   (uint16)                         │  2 bytes
 * ├──────────────────────────────────────────┤
 * │ idBytes (char[idLen])                    │  idLen bytes (UTF-8)
 * ├──────────────────────────────────────────┤
 * │ pwLen   (uint16)                         │  2 bytes
 * ├──────────────────────────────────────────┤
 * │ pwBytes (char[pwLen])                    │  pwLen bytes (UTF-8)
 * └──────────────────────────────────────────┘
 *
 *  Example:
 *    id = "test" (4 bytes)
 *    pw = "test" (4 bytes)
 *
 *    Body (hex):
 *      04 00 74 65 73 74  04 00 74 65 73 74
 *
 *    bodyLen = 12 bytes
 */

#include "net/packet/Packet.h"

enum class PacketVersion : uint8_t {
    V1 = 1,
};

enum class Opcode : uint8_t {
    LOGIN_REQ = 0x10,   // 16
    LOGIN_RES_SUCCESS = 0x11,   // 17
    LOGIN_RES_FAIL = 0x12,

    AUTH_REQ = 0x13,   // 18
    AUTH_RES = 0x14,   // 19

    MOVE = 0x20,   // 32

    ATTACK = 0x30,   // 48

    INVENTORY = 0x40,   // 64

    INVALID = 0xFF    // 255
};

#pragma pack(push, 1)
struct CommonPacketHeader {
    PacketVersion version;
    Opcode opcode;
    uint16_t bodyLen;
    uint32_t flags;      //reserved / future use
};
#pragma pack(pop)
