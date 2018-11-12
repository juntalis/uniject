/**
 * @file ctx.c
 * @author Charles Grunwald <cgrunwald@bluewaterads.com>
 */
#include "pch.h"
#include "base_private.h"
#include <uniject/error.h>
#include <uniject/utility.h>

bool unij_init(void)
{
	return unij_memory_init() && unij_module_init();
}

bool unij_shutdown(void)
{
	return unij_memory_shutdown();
}
