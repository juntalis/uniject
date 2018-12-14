/*
 * parg - parse argv
 *
 * Written in 2015-2016 by Joergen Ibsen
 *
 * To the extent possible under law, the author(s) have dedicated all
 * copyright and related and neighboring rights to this software to the
 * public domain worldwide. This software is distributed without any
 * warranty. <http://creativecommons.org/publicdomain/zero/1.0/>
 */
#define _PARG_C_
#include "pch.h"
#include "parg.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/*
 * Check if state is at end of argv.
 */
static int
is_argv_end(const struct parg_state *ps, int argc, wchar_t *const argv[])
{
	return ps->optind >= argc || argv[ps->optind] == NULL;
}

/*
 * Match nextchar against optstring.
 */
static int
match_short(struct parg_state *ps, int argc, wchar_t *const argv[],
            const wchar_t *optstring)
{
	const wchar_t *p = (const wchar_t*)wcschr(optstring, *ps->nextchar);

	if (p == NULL) {
		ps->optopt = *ps->nextchar++;
		return L'?';
	}

	/* If no option argument, return option */
	if (p[1] != L':') {
		return *ps->nextchar++;
	}

	/* If more characters, return as option argument */
	if (ps->nextchar[1] != L'\0') {
		ps->optarg = &ps->nextchar[1];
		ps->nextchar = NULL;
		return *p;
	}

	/* If option argument is optional, return option */
	if (p[2] == L':') {
		return *ps->nextchar++;
	}

	/* Option argument required, so return next argv element */
	if (is_argv_end(ps, argc, argv)) {
		ps->optopt = *ps->nextchar++;
		return optstring[0] == L':' ? L':' : L'?';
	}

	ps->optarg = argv[ps->optind++];
	ps->nextchar = NULL;
	return *p;
}

/*
 * Match string at nextchar against longopts.
 */
static int
match_long(struct parg_state *ps, int argc, wchar_t *const argv[],
           const wchar_t *optstring,
           const struct parg_option *longopts, int *longindex)
{
	size_t len;
	int num_match = 0;
	int match = -1;
	int i;

	len = wcscspn(ps->nextchar, L"=");

	for (i = 0; longopts[i].name; ++i) {
		if (wcsncmp(ps->nextchar, longopts[i].name, len) == 0) {
			match = i;
			num_match++;
			/* Take if exact match */
			if (longopts[i].name[len] == L'\0') {
				num_match = 1;
				break;
			}
		}
	}

	/* Return L'?' on no or ambiguous match */
	if (num_match != 1) {
		ps->optopt = 0;
		ps->nextchar = NULL;
		return L'?';
	}

	assert(match != -1);

	if (longindex) {
		*longindex = match;
	}

	if (ps->nextchar[len] == L'=') {
		/* Option argument present, check if extraneous */
		if (longopts[match].has_arg == PARG_NOARG) {
			ps->optopt = longopts[match].flag ? 0 : longopts[match].val;
			ps->nextchar = NULL;
			return optstring[0] == L':' ? L':' : L'?';
		}
		else {
			ps->optarg = &ps->nextchar[len + 1];
		}
	}
	else if (longopts[match].has_arg == PARG_REQARG) {
		/* Option argument required, so return next argv element */
		if (is_argv_end(ps, argc, argv)) {
			ps->optopt = longopts[match].flag ? 0 : longopts[match].val;
			ps->nextchar = NULL;
			return optstring[0] == L':' ? L':' : L'?';
		}

		ps->optarg = argv[ps->optind++];
	}

	ps->nextchar = NULL;

	if (longopts[match].flag != NULL) {
		*longopts[match].flag = longopts[match].val;
		return 0;
	}

	return longopts[match].val;
}

void
parg_init(struct parg_state *ps)
{
	ps->optarg = NULL;
	ps->optind = 1;
	ps->optopt = L'?';
	ps->nextchar = NULL;
}

int
parg_getopt(struct parg_state *ps, int argc, wchar_t *const argv[],
            const wchar_t *optstring)
{
	return parg_getopt_long(ps, argc, argv, optstring, NULL, NULL);
}

int
parg_getopt_long(struct parg_state *ps, int argc, wchar_t *const argv[],
                 const wchar_t *optstring,
                 const struct parg_option *longopts, int *longindex)
{
	assert(ps != NULL);
	assert(argv != NULL);
	assert(optstring != NULL);

	ps->optarg = NULL;

	if (argc < 2) {
		return -1;
	}

	/* Advance to next element if needed */
	if (ps->nextchar == NULL || *ps->nextchar == L'\0') {
		if (is_argv_end(ps, argc, argv)) {
			return -1;
		}

		ps->nextchar = argv[ps->optind++];

		/* Check for nonoption element (including L'-') */
		if (ps->nextchar[0] != L'-' || ps->nextchar[1] == L'\0') {
			ps->optarg = ps->nextchar;
			ps->nextchar = NULL;
			return 1;
		}

		/* Check for L'--' */
		if (ps->nextchar[1] == L'-') {
			if (ps->nextchar[2] == L'\0') {
				ps->nextchar = NULL;
				return -1;
			}

			if (longopts != NULL) {
				ps->nextchar += 2;

				return match_long(ps, argc, argv, optstring,
				                  longopts, longindex);
			}
		}

		ps->nextchar++;
	}

	/* Match nextchar */
	return match_short(ps, argc, argv, optstring);
}

/*
 * Reverse elements of `v` from `i` to `j`.
 */
static void
reverse(wchar_t *v[], int i, int j)
{
	while (j - i > 1) {
		wchar_t *tmp = v[i];
		v[i] = v[j - 1];
		v[j - 1] = tmp;
		++i;
		--j;
	}
}

/*
 * Reorder elements of `argv` with no special cases.
 *
 * This function assumes there is no `--` element, and the last element
 * is not an option missing a required argument.
 *
 * The algorithm is described here:
 * http://hardtoc.com/2016/11/07/reordering-arguments.html
 */
static int
parg_reorder_simple(int argc, wchar_t *argv[],
                    const wchar_t *optstring,
                    const struct parg_option *longopts)
{
	struct parg_state ps;
	int change;
	int l = 0;
	int m = 0;
	int r = 0;

	if (argc < 2) {
		return argc;
	}

	do {
		int nextind;
		int c;

		parg_init(&ps);

		nextind = ps.optind;

		/* Parse until end of argument */
		do {
			c = parg_getopt_long(&ps, argc, argv, optstring, longopts, NULL);
		} while (ps.nextchar != NULL && *ps.nextchar != L'\0');

		change = 0;

		do {
			/* Find next non-option */
			for (l = nextind; c != 1 && c != -1;) {
				l = ps.optind;

				do {
					c = parg_getopt_long(&ps, argc, argv, optstring, longopts, NULL);
				} while (ps.nextchar != NULL && *ps.nextchar != L'\0');
			}

			/* Find next option */
			for (m = l; c == 1;) {
				m = ps.optind;

				do {
					c = parg_getopt_long(&ps, argc, argv, optstring, longopts, NULL);
				} while (ps.nextchar != NULL && *ps.nextchar != L'\0');
			}

			/* Find next non-option */
			for (r = m; c != 1 && c != -1;) {
				r = ps.optind;

				do {
					c = parg_getopt_long(&ps, argc, argv, optstring, longopts, NULL);
				} while (ps.nextchar != NULL && *ps.nextchar != L'\0');
			}

			/* Find next option */
			for (nextind = r; c == 1;) {
				nextind = ps.optind;

				do {
					c = parg_getopt_long(&ps, argc, argv, optstring, longopts, NULL);
				} while (ps.nextchar != NULL && *ps.nextchar != L'\0');
			}

			if (m < r) {
				change = 1;
				reverse(argv, l, m);
				reverse(argv, m, r);
				reverse(argv, l, r);
			}
		} while (c != -1);
	} while (change != 0);

	return l + (r - m);
}

int
parg_reorder(int argc, wchar_t *argv[], const wchar_t *optstring, const struct parg_option *longopts)
{
	struct parg_state ps;
	int lastind;
	int optend;
	int c;

	assert(argv != NULL);
	assert(optstring != NULL);

	if (argc < 2) {
		return argc;
	}

	parg_init(&ps);

	/* Find end of normal arguments */
	do {
		lastind = ps.optind;

		c = parg_getopt_long(&ps, argc, argv, optstring, longopts, NULL);

		/* Check for trailing option with error */
		if ((c == L'?' || c == L':') && is_argv_end(&ps, argc, argv)) {
			lastind = ps.optind - 1;
			break;
		}
	} while (c != -1);

	optend = parg_reorder_simple(lastind, argv, optstring, longopts);

	/* Rotate `--` or trailing option with error into position */
	if (lastind < argc) {
		reverse(argv, optend, lastind);
		reverse(argv, optend, lastind + 1);
		++optend;
	}

	return optend;
}
