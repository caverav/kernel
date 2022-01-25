// Copyright (C) 2021  ilobilo

#pragma once

#include <drivers/net/nicmgr/nicmgr.hpp>
#include <stdint.h>

using namespace kernel::drivers::net;

namespace kernel::system::net::ipv4 {

#define IPv4_VER 4
#define IPv4_TTL 64
#define IPv4_TRIES 3

enum ipv4Prot
{
    IPv4_PROT_ICMP = 1,
    IPv4_PROT_TCP = 6,
    IPv4_PROT_UDP = 17,
};

struct [[gnu::packed]] ipv4Hdr
{
    uint8_t version : 4;
    uint8_t ihl : 4;
    uint8_t tos;
    uint16_t len;
    uint16_t id;
    uint16_t flags : 3;
    uint16_t frag_offset : 13;
    uint8_t ttl;
    uint8_t proto;
    uint16_t csum;
    uint8_t sip[4];
    uint8_t dip[4];
    uint8_t data[];
};

extern bool debug;

void send(nicmgr::NIC *nic, uint8_t *dip, void *data, size_t length, ipv4Prot protocol);
void receive(nicmgr::NIC *nic, ipv4Hdr *packet);
}