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
 *  strsort - sort an array pointers using fn
 *
 *	fn follows strcmp(3) conventions
 *
 *   David Korn
 *   AT&T Bell Laboratories
 *
 *  derived from Bourne Shell
 */

#include <ast.h>

void
strsort(char** argv, int n, int(*fn)(const char*, const char*))
{
	int 	j;
	char**	ap;
	char*		s;
	int 		k;

	for (j = 1; j <= n; j *= 2);
	for (int m = 2 * j - 1; m /= 2;)
		for (j = 0, k = n - m; j < k; j++)
			for (int i = j; i >= 0; i -= m)
			{
				ap = &argv[i];
				if ((*fn)(ap[m], ap[0]) >= 0) break;
				s = ap[m];
				ap[m] = ap[0];
				ap[0] = s;
			}
}
