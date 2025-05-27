/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2025 Contributors to ksh 93u+m           *
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
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * return formatted <magicid.h> version string
 */

#include <ast.h>

char*
fmtversion(unsigned long v)
{
	char*	cur;
	char*	end;
	char*	buf;
	int	n;

	buf = cur = fmtbuf(n = 18);
	end = cur + n;
	if (v >= 19700101L && v <= 29991231L)
		sfsprintf(cur, (size_t)(end - cur), "%04lu-%02lu-%02lu", (v / 10000) % 10000, (v / 100) % 100, v % 100);
	else
	{
		if (n = (v >> 24) & 0xff)
			cur += sfsprintf(cur, (size_t)(end - cur), "%d.", n);
		if (n = (v >> 16) & 0xff)
			cur += sfsprintf(cur, (size_t)(end - cur), "%d.", n);
		sfsprintf(cur, (size_t)(end - cur), "%ld.%ld", (v >> 8) & 0xff, v & 0xff);
	}
	return buf;
}
