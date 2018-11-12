/**
 * @file base_private.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief TODO: Description for base_private.h
 */
#ifndef _BASE_PRIVATE_H_
#define _BASE_PRIVATE_H_
#pragma once

#include <uniject/base.h>

#ifdef __cplusplus
extern "C" {
#endif

bool unij_memory_init(void);
bool unij_memory_shutdown(void);

bool unij_module_init(void);

#ifdef __cplusplus
};
#endif

#endif /* _BASE_PRIVATE_H_ */
