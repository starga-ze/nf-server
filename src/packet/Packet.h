#pragma once

#include <string>
#include <netinet/in.h>
#include <vector>
#include <cstdint>

enum class Protocol {
    TCP,
    UDP,
    TLS,
    UNKNOWN
};

struct ConnInfo {
    Protocol protocol;
    uint32_t srcIp;
    uint16_t srcPort;
    uint32_t dstIp;
    uint16_t dstPort;
};

class Packet {
public:
    Packet(int fd,
           Protocol proto,
           std::vector <uint8_t> payload,
           const sockaddr_in &srcAddr,
           const sockaddr_in &dstAddr);

    ~Packet();

    std::vector<uint8_t> takePayload();

    std::string dump() const;

    int getFd() const;

    size_t getTxOffset() const;

    const ConnInfo &getConnInfo() const;

    Protocol getProtocol() const;

    uint32_t getSrcIp() const;

    uint16_t getSrcPort() const;

    uint32_t getDstIp() const;

    uint16_t getDstPort() const;

    const std::vector <uint8_t> &getPayload() const;

    void updateTxOffset(size_t bytes);

private:
    int m_fd;
    ConnInfo m_connInfo;
    std::vector <uint8_t> m_payload;
    size_t m_txOffset = 0;
};

