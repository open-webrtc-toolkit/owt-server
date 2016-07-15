/**
 * @file sleep.cpp  System sleep for Symbian
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <e32std.h>

extern "C" {
#include <re_types.h>
#include <re_sys.h>
}


void sys_usleep(unsigned int us)
{
	User::After(us);
}
