/**
 * @file build_config.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief libuniject's build configuration.
 */
#ifndef _UNIJECT_BUILD_CONFIG_H_
#define _UNIJECT_BUILD_CONFIG_H_
#pragma once

/**
 * @def UNIJ_SE_TOKENS
 * Predefined list of privilege token names to acquire.
 * NOTE: If you need to use the Global\ namespace, add SE_CREATE_GLOBAL_NAME to
 * this list.
 */
#define UNIJ_SE_TOKENS { \
		(const wchar_t*)SE_DEBUG_NAME, \
		(const wchar_t*)SE_BACKUP_NAME, \
		(const wchar_t*)SE_RESTORE_NAME \
	}

/**
 * @def UNIJ_OBJECT_SCOPE "Local"
 * The scope-based namespace used when creating kernel objects. (shared memory,
 * named synchronization stuff, etc) Either "Local" or "Global".
 * NOTE: See unij_se_tokens for additional steps if you'd like to use the global
 * namespace.
 */
#define UNIJ_OBJECT_SCOPE "Local"

/**
 * @def UNIJ_OBJECT_PREFIX "uniject"
 * Prefix for all object names that we create. Resulting names will be:
 * ```SCOPE "\\" PREFIX "." TYPE ":" KEY "@" PID```
 */
#define UNIJ_OBJECT_PREFIX "uniject"

/**
 * @def UNIJ_PARAMS_KEY "params"
 * @brief The "key" part of the object name for storing our params in shared memory.
 * See uniject/params.h for more details.
 */
#define UNIJ_PARAMS_KEY "params"


/**
 * @brief Max size to assign when creating the heap used for all the library's allocations (minuse the ipc)
 * Defaults to 0, so that it can grow as necessary.
 */
#define UNIJ_HEAP_MAXSIZE (0)

#endif /* _UNIJECT_BUILD_CONFIG_H_ */
