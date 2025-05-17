/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2022 Contributors to ksh 93u+m           *
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

#include <tv.h>
#include <tm.h>

/*
 * Tv_t fmttime()
 */

char*
fmttv(const char* fmt, Tv_t* tv)
{
	char*	s;
	char*	t;
	size_t	n;

	s = fmttime(fmt, (time_t)tv->tv_sec);
	if (!tv->tv_nsec || tv->tv_nsec == TV_NSEC_IGNORE)
		return s;
	t = fmtbuf(n = strlen(s) + 11);
	sfsprintf(t, n, "%s.%09lu", s, (unsigned long)tv->tv_nsec);
	return t;
}
