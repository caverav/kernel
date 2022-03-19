// Copyright (C) 2021-2022  ilobilo

#include <system/cpu/smp/smp.hpp>

void errno_set(errno err)
{
    if (this_thread()) this_thread()->err = err;
    else this_cpu->err = err;
}

errno errno_get()
{
    if (this_thread()) return this_thread()->err;
    return this_cpu->err;
}