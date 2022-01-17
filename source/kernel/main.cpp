// Copyright (C) 2021  ilobilo

#pragma region include
#include <drivers/display/framebuffer/framebuffer.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <system/sched/scheduler/scheduler.hpp>
#include <drivers/block/drivemgr/drivemgr.hpp>
#include <drivers/ps2/keyboard/keyboard.hpp>
#include <drivers/net/rtl8139/rtl8139.hpp>
#include <drivers/net/rtl8169/rtl8169.hpp>
#include <system/cpu/syscall/syscall.hpp>
#include <drivers/net/nicmgr/nicmgr.hpp>
#include <drivers/display/ssfn/ssfn.hpp>
#include <drivers/audio/pcspk/pcspk.hpp>
#include <drivers/display/ssfn/ssfn.hpp>
#include <drivers/net/e1000/e1000.hpp>
#include <drivers/block/ahci/ahci.hpp>
#include <drivers/ps2/mouse/mouse.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <drivers/fs/devfs/devfs.hpp>
#include <system/sched/hpet/hpet.hpp>
#include <drivers/vmware/vmware.hpp>
#include <system/sched/pit/pit.hpp>
#include <system/sched/rtc/rtc.hpp>
#include <system/cpu/apic/apic.hpp>
#include <system/cpu/gdt/gdt.hpp>
#include <system/cpu/idt/idt.hpp>
#include <system/cpu/smp/smp.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <system/mm/vmm/vmm.hpp>
#include <system/acpi/acpi.hpp>
#include <system/pci/pci.hpp>
#include <kernel/kernel.hpp>
#include <apps/kshell.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>
#include <lib/panic.hpp>
#include <lib/buddy.hpp>
#include <lib/log.hpp>
#include <stivale2.h>
#pragma endregion include

using namespace kernel::drivers::display;
using namespace kernel::drivers::audio;
using namespace kernel::drivers::block;
using namespace kernel::drivers::net;
using namespace kernel::drivers::fs;
using namespace kernel::drivers;
using namespace kernel::system::sched;
using namespace kernel::system::cpu;
using namespace kernel::system::mm;
using namespace kernel::system;

namespace kernel {

void time()
{
    while (true)
    {
        size_t size = 0;
        for (size_t i = 0; i < STACK_SIZE; i++)
        {
            if (kernel_stack[i] != 'A') break;
            size++;
        }

        uint64_t free = pmm::freemem() / 1024;

        ssfn::setcolour(ssfn::fgcolour, 0x227AD3);
        ssfn::printfat(0, 0, "\rCurrent RTC time: %s", rtc::getTime());
        ssfn::printfat(0, 1, "\rMaximum stack usage: %zu Bytes", STACK_SIZE - size);
        ssfn::printfat(0, 2, "\rFree RAM: %ld KB", free);
    }
}

extern "C" void (*__init_array_start)(), (*__init_array_end)();
void constructors_init()
{
    for (void (**ctor)() = &__init_array_start; ctor < &__init_array_end; ctor++) (*ctor)();
}

void main()
{
    log("Welcome to kernel project");
    terminal::center("Welcome to kernel project");

    if (!strcmp(KERNEL_VERSION, "0")) log("Git version: %s\n", GIT_VERSION);
    else log("Version: %s\n", KERNEL_VERSION);
    if (!strcmp(KERNEL_VERSION, "0")) printf("Git version: %s\n", GIT_VERSION);
    else printf("Version: %s\n", KERNEL_VERSION);

    log("CPU cores available: %ld", smp_tag->cpu_count);
    log("Total usable memory: %ld MB\n", getmemsize() / 1024 / 1024);
    printf("CPU cores available: %ld\n", smp_tag->cpu_count);
    printf("Total usable memory: %ld MB\n", getmemsize() / 1024 / 1024);

    log("Kernel cmdline: %s", cmdline);
    log("Available kernel modules:");
    for (size_t i = 0; i < mod_tag->module_count; i++)
    {
        log("%zu) %s", i, mod_tag->modules[i].string);
    }
    serial::newline();

    terminal::check("Initialising PMM...", reinterpret_cast<uint64_t>(pmm::init), -1, pmm::initialised);
    terminal::check("Initialising VMM...", reinterpret_cast<uint64_t>(vmm::init), -1, vmm::initialised);
    constructors_init();
    terminal::check("Initialising GDT...", reinterpret_cast<uint64_t>(gdt::init), -1, gdt::initialised);
    terminal::check("Initialising IDT...", reinterpret_cast<uint64_t>(idt::init), -1, idt::initialised);

    terminal::check("Initialising ACPI...", reinterpret_cast<uint64_t>(acpi::init), -1, acpi::initialised);
    terminal::check("Initialising HPET...", reinterpret_cast<uint64_t>(hpet::init), -1, hpet::initialised);
    terminal::check("Initialising PIT...", reinterpret_cast<uint64_t>(pit::init), -1, pit::initialised);
    terminal::check("Initialising PCI...", reinterpret_cast<uint64_t>(pci::init), -1, pci::initialised);
    terminal::check("Initialising APIC...", reinterpret_cast<uint64_t>(apic::init), -1, apic::initialised);
    terminal::check("Initialising SMP...", reinterpret_cast<uint64_t>(smp::init), -1, smp::initialised);

    terminal::check("Initialising AHCI...", reinterpret_cast<uint64_t>(ahci::init), -1, ahci::initialised);

    terminal::check("Initialising RTL8139...", reinterpret_cast<uint64_t>(rtl8139::init), -1, rtl8139::initialised);
    terminal::check("Initialising RTL8169...", reinterpret_cast<uint64_t>(rtl8169::init), -1, rtl8169::initialised);
    terminal::check("Initialising E1000...", reinterpret_cast<uint64_t>(e1000::init), -1, e1000::initialised);

    terminal::check("Initialising Drive Manager...", reinterpret_cast<uint64_t>(drivemgr::init), -1, drivemgr::initialised);
    terminal::check("Initialising NIC Manager...", reinterpret_cast<uint64_t>(nicmgr::init), -1, nicmgr::initialised);

    terminal::check("Initialising VFS...", reinterpret_cast<uint64_t>(vfs::init), -1, vfs::initialised);
    int m = find_module("initrd");
    terminal::check("Initialising Initrd...", reinterpret_cast<uint64_t>(ustar::init), mod_tag->modules[m].begin, ustar::initialised, (m != -1 && strstr(cmdline, "initrd")));
    terminal::check("Initialising DEVFS...", reinterpret_cast<uint64_t>(devfs::init), -1, devfs::initialised);

    terminal::check("Initialising System Calls...", reinterpret_cast<uint64_t>(syscall::init), -1, syscall::initialised);

    terminal::check("Initialising PS/2 Keyboard...", reinterpret_cast<uint64_t>(ps2::kbd::init), -1, ps2::kbd::initialised);
    terminal::check("Initialising PS/2 Mouse...", reinterpret_cast<uint64_t>(ps2::mouse::init), -1, ps2::mouse::initialised, (!strstr(cmdline, "nomouse")));

    terminal::check("Initialising VMWare Tools...", reinterpret_cast<uint64_t>(vmware::init), -1, vmware::initialised);

    printf("Current RTC time: %s\n\n", rtc::getTime());
    printf("Userspace has not been implemented yet! dropping to kernel shell...\n\n");

    scheduler::proc_create("Init", reinterpret_cast<uint64_t>(apps::kshell::run), 0);
    scheduler::thread_create(reinterpret_cast<uint64_t>(time), 0, scheduler::initproc);

    scheduler::init();
}
}