// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <drivers/display/terminal/printf.h>
#include <lib/math.hpp>
#include <cstdint>
#include <stdarg.h>

namespace kernel::drivers::display::terminal {

extern bool initialised;
extern char *colour;

void print(const char *string);
void printi(int i);
void printc(char c);

void init();

void cursor_up(int lines = 1), cursor_down(int lines = 1), cursor_right(int lines = 1), cursor_left(int lines = 1);

void reset();
void clear(const char *ansii_colour = colour);
void setcolour(const char *ascii_colour), resetcolour();
void center(const char *text);

void callback(uint64_t type, uint64_t first, uint64_t second, uint64_t third);
point getpos();

void check(const char *message, uint64_t init, int64_t args, bool &ok, bool shouldinit = true);
}