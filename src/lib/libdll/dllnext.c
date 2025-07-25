/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1997-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2025 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  Martijn Dekker <martijn@inlv.org>                   *
*                                                                      *
***********************************************************************/
/*
 * Glenn Fowler
 * AT&T Research
 */

#include <ast.h>
#include <dlldefs.h>

#if _hdr_rld_interface
#include <rld_interface.h>
#endif

/*
 * return a handle for the next layer down,
 * i.e., the next layer that has symbols covered
 * by the main prog and dll's loaded so far
 *
 * intentionally light on external lib calls
 * so this routine can be used early in process
 * startup
 */

#ifdef	_DLL_RLD_SYM

#define DEBUG		1

#if DEBUG

typedef ssize_t (*Write_f)(int, const void*, size_t);

#endif

#undef	dllnext

void*
_dll_next(int flags, _DLL_RLD_SYM_TYPE* here)
{
	char*		vp;
	void*		lp;
	int		found = 0;
	char*		s;
	char*		b;
	char*		e;
	char		dummy[256];
#if DEBUG
	Write_f		wr = 0;
	Write_f		xr;
	char		buf[1024];
#endif

#if DEBUG
	if (getenv("DLL_DEBUG") && (vp = (char*)_rld_new_interface(_RLD_FIRST_PATHNAME)))
	{
		do
		{
			if (strcmp(vp, "MAIN") && (lp = dllopen(vp, flags)))
			{
				if (xr = (Write_f)dlsym(lp, "write"))
					wr = xr;
			}
		} while (vp = (char*)_rld_new_interface(_RLD_NEXT_PATHNAME));
	}
#endif
	if (vp = (char*)_rld_new_interface(_RLD_FIRST_PATHNAME))
	{
		do
		{
			if (lp = dllopen(strcmp(vp, "MAIN") ? vp : NULL, flags))
			{
				if (found)
				{
					b = e = 0;
					s = vp;
					for (;;)
					{
						switch (*s++)
						{
						case 0:
							break;
						case '/':
							b = s;
							e = 0;
							continue;
						case '.':
							if (!e)
								e = s - 1;
							continue;
						default:
							continue;
						}
						break;
					}
					if (b && e)
					{
						s = dummy;
						*s++ = '_';
						*s++ = '_';
						while (b < e)
							*s++ = *b++;
						b = "_dummy";
						while (*s++ = *b++);
						if (dlsym(lp, dummy))
						{
							dlclose(lp);
							lp = 0;
						}
					}
					if (lp)
					{
#if DEBUG
						if (wr)
							(*wr)(2, buf, sfsprintf(buf, sizeof(buf), "dll: next %s\n", vp));
#endif
						return lp;
					}
#if DEBUG
					else if (wr)
						(*wr)(2, buf, sfsprintf(buf, sizeof(buf), "dll: skip %s\n", vp));
#endif
				}
				else if ((_DLL_RLD_SYM_TYPE*)dlsym(lp, _DLL_RLD_SYM_STR) == here)
				{
#if DEBUG
					if (wr)
						(*wr)(2, buf, sfsprintf(buf, sizeof(buf), "dll: this %s\n", vp));
#endif
					found = 1;
				}
			}
		} while (vp = (char*)_rld_new_interface(_RLD_NEXT_PATHNAME));
	}
	return dllnext(flags);
}

#endif

#ifndef RTLD_NEXT
#if _dll_DYNAMIC

#include <link.h>

extern struct link_dynamic	_DYNAMIC;

#endif
#endif

void*
dllnext(int flags)
{
	void*			dll;
#ifndef RTLD_NEXT
#if _dll_DYNAMIC
	char*			s;
	char*			b;
#endif
	char*			ver;
	char*				path;

	static char			next[] = { _DLL_NEXT_PATH };
#endif

#ifdef RTLD_NEXT
	NOT_USED(flags);
	dll = RTLD_NEXT;
#else
	path = next;
#if _dll_DYNAMIC
	for (struct link_map *map = _DYNAMIC.ld_un.ld_1->ld_loaded; map; map = map->lm_next)
	{
		b = 0;
		s = map->lm_name;
		while (*s)
			if (*s++ == '/')
				b = s;
		if (b && b[0] == 'l' && b[1] == 'i' && b[2] == 'b' && b[3] == 'c' && b[4] == '.')
		{
			path = map->lm_name;
			break;
		}
	}
#endif
	ver = path + strlen(path);
	while (!(dll = dllopen(path, flags)))
	{
		do
		{
			if (ver <= path)
				return NULL;
		} while (*--ver != '.');
		if (*(ver + 1) <= '0' || *(ver + 1) >= '9')
			return NULL;
		*ver = 0;
	}
#endif
	return dll;
}
