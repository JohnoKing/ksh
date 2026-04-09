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
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
/*
 * Glenn Fowler
 * AT&T Research
 *
 * token stream routines
 */

#include <ast.h>
#include <tok.h>

#define FLG_RESTORE	01		/* restore string on close	*/
#define FLG_NEWLINE	02		/* return newline token next	*/

typedef struct Tok_s			/* token stream state		*/
{
	union
	{
	char*		end;		/* end ('\0') of last token	*/
	struct Tok_s*	nxt;		/* next in free list		*/
	}		ptr;
	char		chr;		/* replace *end with this	*/
	char		flg;		/* FLG_*			*/
} Tok_t;

static Tok_t*		freelist;

/*
 * open a new token stream on s
 * if f==0 then string is not restored
 */

char*
tokopen(char* s, int f)
{
	Tok_t*	p;

	if (p = freelist)
		freelist = freelist->ptr.nxt;
	else if (unlikely(!(p = newof(0, Tok_t, 1, 0))))
		return NULL;
	p->chr = *(p->ptr.end = s);
	p->flg = f ? FLG_RESTORE : 0;
	return (char*)p;
}

/*
 * close a token stream
 * restore the string to its original state
 */

void
tokclose(char* u)
{
	Tok_t*	p = (Tok_t*)u;

	if (p->flg == FLG_RESTORE && *p->ptr.end != p->chr)
		*p->ptr.end = p->chr;
	p->ptr.nxt = freelist;
	freelist = p;
}

/*
 * return next space separated token
 * "\n" is returned as a token
 * 0 returned when no tokens remain
 * "..." and '...' quotes are honored with \ escapes
 */

char*
tokread(char* u)
{
	Tok_t*	p = (Tok_t*)u;
	char*	s;
	char*	r;
	int	q;
	int	c;

	/*
	 * restore string on each call
	 */

	if (!p->chr)
		return NULL;
	s = p->ptr.end;
	switch (p->flg)
	{
	case FLG_NEWLINE:
		p->flg = 0;
		return "\n";
	case FLG_RESTORE:
		if (*s != p->chr)
			*s = p->chr;
		break;
	default:
		if (!*s)
			s++;
		break;
	}

	/*
	 * skip leading space
	 */

	while (*s == ' ' || *s == '\t')
		s++;
	if (!*s)
	{
		p->ptr.end = s;
		p->chr = 0;
		return NULL;
	}

	/*
	 * find the end of this token
	 */

	r = s;
	q = 0;
	for (;;)
		switch (c = *r++)
		{
		case '\n':
			if (!q)
			{
				if (s == (r - 1))
				{
					if (!p->flg)
					{
						p->ptr.end = r;
						return "\n";
					}
					r++;
				}
				else if (!p->flg)
					p->flg = FLG_NEWLINE;
			}
			/* FALLTHROUGH */
		case ' ':
		case '\t':
			if (q)
				break;
			/* FALLTHROUGH */
		case 0:
			if (s == --r)
			{
				p->ptr.end = r;
				p->chr = 0;
			}
			else
			{
				p->chr = *(p->ptr.end = r);
				if (*r)
					*r = 0;
			}
			return s;
		case '\\':
			if (*r)
				r++;
			break;
		case '"':
		case '\'':
			if (c == q)
				q = 0;
			else if (!q)
				q = c;
			break;
		}
}
