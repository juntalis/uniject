/**
 * @file winget_args.h
 * 
 * TODO: Description
 */
#ifndef _UNIJECT_ARGS_H_
#define _UNIJECT_ARGS_H_
#pragma once

#include <uniject.h>
#include <uniject/params.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct unij_cliargs unij_cliargs_t;

/* customized structure for command line parameters */
struct unij_cliargs
{
	bool help : 1;
	bool list : 1;
	unij_params_t params;
};

/* function prototypes */
void parse_args(unij_cliargs_t *argsobj, int argc, wchar_t* argv[]);
UNIJ_NORETURN usage(int status, const wchar_t* program_name, bool show);

#ifdef __cplusplus
}
#endif

#endif /* _UNIJECT_ARGS_H_ */
