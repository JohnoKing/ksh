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
* AT&T Bell Laboratories
*/

#include "univlib.h"

/*
 * create symbolic name from external representation text in buf
 * the arg order matches link(2)
 */

int
pathsetlink(const char* buf, const char* name)
{
	char*	t = (char*)buf;
#ifdef UNIV_MAX
	char*	s = (char*)buf;
	char	tmp[PATH_MAX];

	while (*s)
	{
		if (*s++ == univ_cond[0] && !strncmp(s - 1, univ_cond, univ_size))
		{
			s--;
			t = tmp;
			for (int n = 0; n < UNIV_MAX; n++)
				if (*univ_name[n])
			{
				*t++ = ' ';
#ifdef ATT_UNIV
				*t++ = '1' + n;
				*t++ = ':';
#else
				for (char *v = univ_name[n]; *t = *v++; t++);
				*t++ = '%';
#endif
				for (char *v = (char*)buf; v < s; *t++ = *v++);
				for (char *v = univ_name[n]; *t = *v++; t++);
				for (char *v = s + univ_size; *t = *v++; t++);
			}
			t = tmp;
			break;
		}
	}
#endif
	return symlink(t, name);
}
