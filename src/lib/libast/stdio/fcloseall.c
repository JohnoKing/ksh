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

#include "stdhdr.h"

#define MAXLOOP		3

int
fcloseall(void)
{
	Sfpool_t*	next;
	int		nclose;
	int		count;

	for(int loop = 0; loop < MAXLOOP; ++loop)
	{	nclose = count = 0;
		for(Sfpool_t *p = &_Sfpool; p; p = next)
		{	/* find the next legitimate pool */
			for(next = p->next; next; next = next->next)
				if(next->n_sf > 0)
					break;
			for(int n = 0; n < ((p == &_Sfpool) ? p->n_sf : 1); ++n)
			{	count += 1;
				if(sfclose(p->sf[n]) >= 0)
					nclose += 1;
			}
		}
		if(nclose == count)
			break;
	}
	return 0; /* always return 0 per GNU */
}
