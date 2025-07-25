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
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/

#include "asohdr.h"

int
asolock(unsigned int volatile* lock, unsigned int key, int type)
{
	if (key)
		switch (type)
		{
		case ASO_UNLOCK:
			return *lock == 0 ? 0 : asocasint(lock, key, 0) == key ? 0 : -1;
		case ASO_TRYLOCK:
			return *lock == key ? 0 : asocasint(lock, 0, key) == 0 ? 0 : -1;
		case ASO_LOCK:
			if (*lock == key)
				return 0;
			/* FALLTHROUGH */
		case ASO_SPINLOCK:
			for (unsigned int k = 0; asocasint(lock, 0, key) != 0; ASOLOOP(k));
			return 0;
		}
	return -1;
}
