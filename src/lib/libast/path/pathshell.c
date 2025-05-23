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
 * G. S. Fowler
 * D. G. Korn
 * AT&T Bell Laboratories
 *
 * shell library support
 */

#include <ast.h>
#include <sys/stat.h>

/*
 * return pointer to the full path name of the shell
 *
 * SHELL is read from the environment and must start with /
 *
 * if setuid or setgid then the executable and its containing
 * directory must not be owned by the real user/group
 *
 * root/administrator has its own test
 *
 * astconf("SH",NULL,NULL) is returned by default
 *
 * NOTE: csh is rejected because the bsh/csh differentiation is
 *       not done for `csh script arg ...'
 */

#ifdef _WINIX
#	define EXE "?(.exe)"
#else
#	define EXE
#endif

char*
pathshell(void)
{
	char*		sh;
	uid_t		ru;
	uid_t		eu;
	gid_t		rg;
	gid_t		eg;
	struct stat	st;

	static char*	val;

	if ((sh = getenv("SHELL")) && *sh == '/' && strmatch(sh, "*/(sh|*[!cC]sh)*([[:digit:]])?(-+([.[:alnum:]]))" EXE))
	{
		if (!(ru = getuid()) || !eaccess("/bin", W_OK))
		{
			if (stat(sh, &st))
				goto defshell;
			if (ru != st.st_uid && !strmatch(sh, "?(/usr)?(/local)/?([ls])bin/?([[:lower:]])sh" EXE))
				goto defshell;
		}
		else
		{
			eu = geteuid();
			rg = getgid();
			eg = getegid();
			if (ru != eu || rg != eg)
			{
				char*	s;
				char	dir[PATH_MAX];

				s = sh;
				for (;;)
				{
					if (stat(s, &st))
						goto defshell;
					if (ru != eu && st.st_uid == ru)
						goto defshell;
					if (rg != eg && st.st_gid == rg)
						goto defshell;
					if (s != sh)
						break;
					if (strlen(s) >= sizeof(dir))
						goto defshell;
					strcpy(dir, s);
					if (!(s = strrchr(dir, '/')))
						break;
					*s = 0;
					s = dir;
				}
			}
		}
		return sh;
	}
 defshell:
	if (!(sh = val))
	{
		if (!*(sh = astconf("SH", NULL, NULL)) || *sh != '/' || eaccess(sh, X_OK) || !(sh = strdup(sh)))
			sh = "/bin/sh";
		val = sh;
	}
	return sh;
}
