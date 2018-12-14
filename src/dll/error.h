/**
 * @file error.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief TODO: Description for error.h
 */
#ifndef _LOADER_ERROR_H_
#define _LOADER_ERROR_H_
#pragma once

#include <uniject.h>
#include <uniject/error.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct unij_errors unij_errors_t;

struct unij_errors
{
	unij_error_t unij_code;
	uint32_t win32_code;
};

bool unij_loader_thread(void);

void unij_error_init(void);
void unij_error_shutdown(void);

void unij_set_fatal_error(unij_errors_t codes);
unij_errors_t unij_get_fatal_error(void);

#ifdef __cplusplus
};
#endif

#endif /* _LOADER_ERROR_H_ */
