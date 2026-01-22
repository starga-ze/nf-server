#pragma once

/*
 *       Common Packet Structure (v2)
 *
 * ┌─────────────────────────────────────┐
 * │    CommonPacketHeader (16 bytes)    │
 * │ ┌────────┬───────────┬────────────┐ │
 * │ │ ver(1) │ opcode(1) │ bodyLen(2) │ │
 * │ └────────┴───────────┴────────────┘ │
 * │            sessionId (8)            │
 * │      flags / reserved (4 bytes)     │
 * ├─────────────────────────────────────┤
 * │      Body (N bytes = bodyLen)       │
 * └─────────────────────────────────────┘
 */

/*
 *          LOGIN REQ BODY (0x10)
 *
 *  SessionId MUST be 0 for LOGIN_REQ
 *
 * ┌─────────────────────────────────────┐
 * │ idLen   (uint16)                    │
 * ├─────────────────────────────────────┤
 * │ idBytes (char[idLen])               │
 * ├─────────────────────────────────────┤
 * │ pwLen   (uint16)                    │
 * ├─────────────────────────────────────┤
 * │ pwBytes (char[pwLen])               │
 * └─────────────────────────────────────┘
 */

/*
 *      LOGIN RES SUCCESS BODY (0x11)
 *
 *  SessionId is delivered via CommonPacketHeader.
 *  Client must send including the sessionId
 *
 * ┌─────────────────────────────────────┐
 * │ resultCode (uint8) = 1              │
 * ├─────────────────────────────────────┤
 * │ optional fields (future)            │
 * └─────────────────────────────────────┘
 */

/*
 *        LOGIN RES FAIL BODY (0x12)
 *
 * ┌─────────────────────────────────────┐
 * │ resultCode (uint8) = 0              │
 * ├─────────────────────────────────────┤
 * │ errorCode (uint16)                  │
 * └─────────────────────────────────────┘
 */


#include "net/packet/Packet.h"

enum class PacketVersion : uint8_t {
    V1 = 1,
};

enum class Opcode : uint8_t {
    LOGIN_REQ = 0x10,   // 16
    LOGIN_RES_SUCCESS = 0x11,   // 17
    LOGIN_RES_FAIL = 0x12,

    LOGOUT_REQ = 0x15,
    LOGOUT_RES_SUCCESS = 0x16,
    LOGOUT_RES_FAIL = 0x17,

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
