// Copyright (C) 2021  ilobilo

#pragma once

#include <system/pci/pci.hpp>
#include <stdint.h>

using namespace kernel::system;

namespace kernel::drivers::net::rtl8139 {

class RTL8139
{
    private:
    pci::pcidevice_t *pcidevice;
    
    uint16_t IOBase = 0;
    uint8_t BARType = 0;
    uint8_t *RXBuffer = nullptr;
    uint32_t current_packet = 0;

    uint8_t TSAD[4] = { 0x20, 0x24, 0x28, 0x2C };
    uint8_t TSD[4] = { 0x10, 0x14, 0x18, 0x1C };
    size_t curr_tx = 0;

    public:
    uint8_t MAC[6];

    void send(void *data, uint64_t length);
    void recive();

    void reset();
    void activate();

    uint16_t status();
    void irq_reset();
    void read_mac();

    RTL8139(pci::pcidevice_t *pcidevice);
};

extern bool initialised;
extern vector<RTL8139*> devices;

void init();
}