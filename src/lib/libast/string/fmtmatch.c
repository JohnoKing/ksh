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
 * AT&T Research
 *
 * return strmatch() expression given REG_AUGMENTED RE
 * 0 returned for invalid RE
 */

#include <ast.h>

char*
fmtmatch(const char* as)
{
	char*	s = (char*)as;
	int	c;
	char*	t;
	char**	p;
	char*	b;
	char*	x;
	char*	y;
	char*	z;
	int	a;
	int	e;
	int	n;
	char*	buf;
	char*	stack[32];

	buf = fmtbuf(3 * (strlen(s) + 1));
	t = b = buf + 3;
	p = stack;
	if (a = *s == '^')
		s++;
	e = 0;
	for (;;)
	{
		switch (c = *s++)
		{
		case 0:
			break;
		case '\\':
			if (!(c = *s++))
				return NULL;
			switch (*s)
			{
			case '*':
			case '+':
			case '?':
				*t++ = *s++;
				*t++ = '(';
				*t++ = '\\';
				*t++ = c;
				c = ')';
				break;
			case '|':
			case '&':
				if (c == '(')
				{
					*t++ = c;
					c = *s++;
					goto logical;
				}
				break;
			case '{':
			case '}':
				break;
			default:
				*t++ = '\\';
				break;
			}
			*t++ = c;
			continue;
		case '[':
			x = t;
			*t++ = c;
			if ((c = *s++) == '^')
			{
				*t++ = '!';
				c = *s++;
			}
			else if (c == '!')
			{
				*t++ = '\\';
				*t++ = c;
				c = *s++;
			}
			for (;;)
			{
				if (!(*t++ = c))
					return NULL;
				if (c == '\\')
					*t++ = c;
				if ((c = *s++) == ']')
				{
					*t++ = c;
					break;
				}
			}
			switch (*s)
			{
			case '*':
			case '+':
			case '?':
				for (y = t + 2, t--; t >= x; t--)
					*(t + 2) = *t;
				*++t = *s++;
				*++t = '(';
				t = y;
				*t++ = ')';
				break;
			}
			continue;
		case '(':
			if (p >= &stack[elementsof(stack)])
				return NULL;
			*p++ = t;
			if (*s == '?')
			{
				s++;
				if (*s == 'K' && *(s + 1) == ')')
				{
					s += 2;
					p--;
					while (*t = *s)
						t++, s++;
					continue;
				}
				*t++ = '~';
			}
			else
				*t++ = '@';
			*t++ = '(';
			continue;
		case ')':
			if (p == stack)
				return NULL;
			p--;
			*t++ = c;
			switch (*s)
			{
			case 0:
				break;
			case '*':
			case '+':
			case '?':
			case '!':
				**p = *s++;
				if (*s == '?')
				{
					s++;
					x = *p + 1;
					for (y = ++t; y > x; y--)
						*y = *(y - 1);
					*x = '-';
				}
				continue;
			case '{':
				for (z = s; *z != '}'; z++)
					if (!*z)
						return NULL;
				n = (int)(z - s);
				if (*++z == '?')
					n++;
				x = *p + n;
				for (y = t += n; y > x; y--)
					*y = *(y - n);
				for (x = *p; s < z; *x++ = *s++);
				if (*s == '?')
				{
					s++;
					*x++ = '-';
				}
				continue;
			default:
				continue;
			}
			break;
		case '.':
			switch (*s)
			{
			case 0:
				*t++ = '?';
				break;
			case '*':
				s++;
				*t++ = '*';
				e = !*s;
				continue;
			case '+':
				s++;
				*t++ = '?';
				*t++ = '*';
				continue;
			case '?':
				s++;
				*t++ = '?';
				*t++ = '(';
				*t++ = '?';
				*t++ = ')';
				continue;
			default:
				*t++ = '?';
				continue;
			}
			break;
		case '*':
		case '+':
		case '?':
		case '{':
			n = *(t - 1);
			if (t == b || n == '(' || n == '|')
				return NULL;
			*(t - 1) = c;
			if (c == '{')
			{
				for (z = s; *z != '}'; z++)
					if (!*z)
						return NULL;
				for (; s <= z; *t++ = *s++);
			}
			if (*s == '?')
			{
				s++;
				*t++ = '-';
			}
			*t++ = '(';
			*t++ = n;
			*t++ = ')';
			continue;
		case '|':
		case '&':
			if (t == b || *(t - 1) == '(')
				return NULL;
		logical:
			if (!*s || *s == ')')
				return NULL;
			if (p == stack && b == buf + 3)
			{
				*--b = '(';
				*--b = '@';
			}
			*t++ = c;
			continue;
		case '$':
			if (e = !*s)
				break;
			/* FALLTHROUGH */
		default:
			*t++ = c;
			continue;
		}
		break;
	}
	if (p != stack)
		return NULL;
	if (b != buf + 3)
		*t++ = ')';
	if (!a && (*b != '*' || *(b + 1) == '(' || (*(b + 1) == '-' || *(b + 1) == '~') && *(b + 2) == '('))
		*--b = '*';
	if (!e)
		*t++ = '*';
	*t = 0;
	return b;
}
