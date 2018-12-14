/**
 * @file process_private.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief TODO: Description for process_private.h
 */
#ifndef _PROCESS_PRIVATE_H_
#define _PROCESS_PRIVATE_H_
#pragma once

#include "peutil.h"
#include <uniject/process.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Used exclusively for injection
 */
struct unij_process
{
	bool needs_free : 1;
	unij_procflags_t flags : 7;
	uint32_t pid;
	unij_wstr_t mono_path;
	HANDLE process;
};

#ifdef __cplusplus
};
#endif

#endif /* _PROCESS_PRIVATE_H_ */
