// Copyright (C) 2021  ilobilo

#include <system/sched/scheduler/scheduler.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/cpu/gdt/gdt.hpp>
#include <system/cpu/smp/smp.hpp>
#include <lib/memory.hpp>

using namespace kernel::drivers::display;
using namespace kernel::system::cpu;

namespace kernel::system::sched::scheduler {

bool initialised = false;
threadentry_t *current_thread;
uint64_t next_pid = 1;
DEFINE_LOCK(thread_lock)

thread_t *alloc(uint64_t addr, void *args)
{
    thread_t *thread = new thread_t;

    acquire_lock(thread_lock);
    thread->pid = next_pid++;
    thread->state = READY;
    thread->stack = (uint8_t*)heap::malloc(TSTACK_SIZE);
    thread->pagemap = vmm::clonePagemap(vmm::kernel_pagemap);
    thread->current_dir = vfs::fs_root->ptr;

    thread->regs.rflags = 0x202;
    thread->regs.cs = 0x28;
    thread->regs.ss = 0x30;

    thread->regs.rip = addr;
    thread->regs.rdi = (uint64_t)args;
    thread->regs.rsp = (uint64_t)(thread->stack + TSTACK_SIZE);
    release_lock(thread_lock);

    return thread;
}

void create(thread_t *thread)
{
    threadentry_t *te = new threadentry_t;
    te->next = current_thread->next;
    current_thread->next = te;
    te->thread = thread;
}

void schedule(registers_t *regs)
{
    if (!current_thread || !initialised || current_thread->thread->killed) return;
    current_thread->thread->regs = *regs;

    while (current_thread->next->thread->killed)
    {
        threadentry_t *oldnext = current_thread->next;
        current_thread->next = current_thread->next->next;
        heap::free(oldnext->thread->stack);
        heap::free(oldnext->thread);
        heap::free(oldnext);
    }
    if (current_thread->thread->state == RUNNING) current_thread->thread->state = READY;
    current_thread = current_thread->next;
    while (current_thread->thread->state != READY) current_thread = current_thread->next; 

    *regs = current_thread->thread->regs;
    vmm::switchPagemap(current_thread->thread->pagemap);
    gdt::set_stack((uintptr_t)current_thread->thread->stack);
    current_thread->thread->state = RUNNING;
}

void block()
{
    asm volatile ("cli");
    current_thread->thread->state = BLOCKED;
    asm volatile ("sti");
}

void block(thread_t *thread)
{
    asm volatile ("cli");
    thread->state = BLOCKED;
    asm volatile ("sti");
}

void unblock(thread_t *thread)
{
    asm volatile ("cli");
    thread->state = READY;
    asm volatile ("sti");
}

void exit()
{
    asm volatile ("cli");
    current_thread->thread->state = KILLED;
    current_thread->thread->killed = true;
    asm volatile ("sti");
}

void exit(thread_t *thread)
{
    asm volatile ("cli");
    thread->state = KILLED;
    thread->killed = true;
    asm volatile ("sti");
}

void init()
{
    serial::info("Initialising scheduler");

    if (initialised)
    {
        serial::warn("Scheduler has already been initialised!\n");
        return;
    }

    current_thread = new threadentry_t;
    current_thread->thread = new thread_t;
    current_thread->next = current_thread;
    current_thread->thread->pagemap = vmm::clonePagemap(vmm::kernel_pagemap);
    current_thread->thread->stack = (uint8_t*)heap::malloc(TSTACK_SIZE);
    current_thread->thread->state = READY;

    serial::newline();
    initialised = true;
}
}