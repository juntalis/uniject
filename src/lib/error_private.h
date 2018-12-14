/**
 * @file error_private.h
 * @author Charles Grunwald <ch@rles.rocks>
 * @brief Internally used helpers for error stuff
 */
#ifndef _ERROR_PRIVATE_H_
#define _ERROR_PRIVATE_H_
#pragma once

#include <uniject/error.h>

#ifdef __cplusplus
extern "C" {
#endif

// Stupidly long names since I'm going to be wrapping these in macros.
static UNIJ_INLINE bool unij_is_required_param_null(void* param, const wchar_t* caller, const wchar_t* name)
{
	bool result = param == NULL;
	if(result) {
		unij_fatal_error(UNIJ_ERROR_ADDRESS, L"Fatal call made to %s: param %s cannot be NULL!", caller, name);
	}
	return result;
}

static UNIJ_INLINE bool unij_is_required_string_invalid(const wchar_t* param, const wchar_t* caller, const wchar_t* name)
{
	bool result = (param == NULL) || (*param == L'\0');
	if(result) {
		unij_fatal_error(UNIJ_ERROR_ADDRESS, L"Fatal call made to %s: param %s cannot be NULL or empty!", caller, name);
	}
	return result;
}

static UNIJ_INLINE bool unij_is_required_handle_invalid(HANDLE param, const wchar_t* caller, const wchar_t* name)
{
	bool result = IS_INVALID_HANDLE(param);
	if(result) {
		unij_fatal_error(UNIJ_ERROR_ADDRESS, L"Fatal call made to %s: param %s must be a valid handle!", caller, name);
	}
	return result;
}

/**
 * @def unij_fatal_alloc()
 * @brief Failed allocation
 */
#define unij_fatal_alloc() \
	unij_fatal_error( \
		UNIJ_ERROR_OUTOFMEMORY, \
		L"Failed allocation at %s:%s:%s", \
		UNIJ_WIDEN(__FILE__), \
		UNIJ_WSTRINGIFY(__LINE__), \
		__FUNCTIONW__ \
	)

/**
 * @def unij_fatal_null(PARAM)
 * @brief Tests if a required pointer param is NULL. Throws a fatal error and returns true if so.
 * @param[in] PARAM The parameter in question.
 * @return ``true`` if the param is ``NULL``. false otherwise.
 */
#define unij_fatal_null(PARAM) \
	(unij_is_required_param_null( \
		(void*)(PARAM), \
		__FUNCTIONW__, \
		UNIJ_WSTRINGIFY(PARAM) \
	))

/**
 * @def unij_fatal_null2(PARAM)
 * @brief \a unij_fatal_null with an override for function name
 * @param[in] PARAM The parameter in question.
 * @param[in] FUNC Function name
 * @return ``true`` if the param is ``NULL``. false otherwise.
 */
#define unij_fatal_null2(PARAM,FUNC) \
	(unij_is_required_param_null( \
		(void*)(PARAM), \
		FUNC, \
		UNIJ_WSTRINGIFY(PARAM) \
	))

/**
 * @def unij_fatal_null3(PARAM)
 * @brief \a unij_fatal_null2 with an override for parameter name
 * @param[in] PARAM The parameter in question.
 * @param[in] FUNC Function name
 * @param[in] NAME Parameter name
 * @return ``true`` if the param is ``NULL``. false otherwise.
 */
#define unij_fatal_null3(PARAM,FUNC,NAME) \
	(unij_is_required_param_null( \
		(void*)(PARAM), \
		FUNC, \
		NAME \
	))

/**
 * @def unij_fatal_string(PARAM)
 * @brief Tests if a required string param is NULL or empty. Throws a fatal error and returns true if so.
 * @param[in] PARAM The parameter in question.
 * @return ``true`` if the param is ``NULL``. false otherwise.
 */
#define unij_fatal_string(PARAM) \
	(unij_is_required_string_invalid( \
		(PARAM), \
		__FUNCTIONW__, \
		UNIJ_WSTRINGIFY(PARAM) \
	))

/**
 * @def unij_fatal_handle(PARAM)
 * @brief Tests if a required HANDLE param is invalid. Throws a fatal error and returns true if so.
 * @param[in] PARAM The parameter in question.
 * @return ``true`` if the param is ``NULL``. false otherwise.
 */
#define unij_fatal_handle(PARAM) \
	(unij_is_required_handle_invalid( \
		(PARAM), \
		__FUNCTIONW__, \
		UNIJ_WSTRINGIFY(PARAM) \
	))


#ifdef __cplusplus
};
#endif

#endif /* _ERROR_PRIVATE_H_ */
