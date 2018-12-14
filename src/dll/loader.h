/**
 * @file loader.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief TODO: Description for loader.h
 */
#ifndef _LOADER_H_
#define _LOADER_H_
#pragma once

#include <uniject.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum unij_error unij_error_t;

unij_error_t loader_main(void);

#ifdef __cplusplus
};
#endif

#endif /* _LOADER_H_ */
