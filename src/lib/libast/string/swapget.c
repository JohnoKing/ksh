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
 * Glenn Fowler
 * AT&T Research
 *
 * internal representation conversion support
 */

#include <ast.h>
#include <swap.h>

/*
 * get int_n from b according to op
 */

intmax_t
swapget(int op, const void* b, int n)
{
	unsigned char*	p;
	unsigned char*	d;
	intmax_t		v;
	unsigned char		tmp[sizeof(intmax_t)];

	if (n > (int)sizeof(intmax_t))
		n = (int)sizeof(intmax_t);
	if (op) swapmem(op, b, d = tmp, n);
	else d = (unsigned char*)b;
	p = d + n;
	v = 0;
	while (d < p)
	{
		v <<= CHAR_BIT;
		v |= *d++;
	}
	return v;
}
