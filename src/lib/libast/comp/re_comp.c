/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2023 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  David Korn <dgk@research.att.com>                   *
*                   Phong Vo <kpv@research.att.com>                    *
*                  Martijn Dekker <martijn@inlv.org>                   *
*                                                                      *
***********************************************************************/
/*
 * re_comp implementation
 */

#include <ast.h>
#include <re_comp.h>
#include <regex.h>

#undef	error
#undef	valid

static struct
{
	char	error[64];
	regex_t	re;
	int	valid;
} state;

char*
re_comp(const char* pattern)
{
	int	r;

	if (unlikely(!pattern || !*pattern))
	{
		if (state.valid)
			return NULL;
		r = REG_BADPAT;
	}
	else
	{
		if (state.valid)
		{
			state.valid = 0;
			regfree(&state.re);
		}
		if (!(r = regcomp(&state.re, pattern, REG_LENIENT|REG_NOSUB|REG_NULL)))
		{
			state.valid = 1;
			return NULL;
		}
	}
	regerror(r, &state.re, state.error, sizeof(state.error));
	return state.error;
}

int
re_exec(const char* subject)
{
	if (state.valid && subject)
		switch (regexec(&state.re, subject, 0, NULL, 0))
		{
		case 0:
			return 1;
		case REG_NOMATCH:
			return 0;
		}
	return -1;
}
