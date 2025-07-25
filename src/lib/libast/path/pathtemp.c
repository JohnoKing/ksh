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
 * AT&T Research
 *
 * generate a temp file / name
 *
 *	[<dir>/][<pfx>]<bas>.<suf>
 *
 * length(<pfx>)<=5
 * length(<bas>)==3
 * length(<suf>)==3
 *
 *	pathtmp(a,b,c,d)	pathtemp(a,L_tmpnam,b,c,0)
 *	tmpfile()		char*p=pathtemp(0,0,0,"tf",&sp);
 *				remove(p);
 *				free(p)
 *	tmpnam(0)		static char p[L_tmpnam];
 *				pathtemp(p,sizeof(p),0,"tn",0)
 *	tmpnam(p)		pathtemp(p,L_tmpnam,0,"tn",0)
 *	tempnam(d,p)		pathtemp(0,d,p,0)
 *	mktemp(p)		pathtemp(0,0,p,0)
 *
 * if buf==0 then space is malloc'd
 * buf size is size
 * dir and pfx may be 0
 * if pfx contains trailing X's then it is a mktemp(3) template
 * otherwise only first 5 chars of pfx are used
 * if fdp!=0 then the path is opened O_EXCL and *fdp is the open fd
 * malloc'd space returned by successful pathtemp() calls
 * must be freed by the caller
 *
 * generated names are pseudorandomized to avoid both
 * collisions and predictions (same alg in sfio/sftmp.c)
 *
 * / as first pfx char provides tmp file generation control
 * 0 returned for unknown ops
 *
 *	/cycle		dir specifies TMPPATH cycle control
 *		automatic	(default) cycled with each tmp file
 *		manual		cycled by application with dir=(nil)
 *		(nil)		cycle TMPPATH
 *	/prefix		dir specifies the default prefix (default AST)
 *	/private	private file/dir modes
 *	/public		public file/dir modes
 *	/TMPPATH	dir overrides the env value
 *	/TMPDIR		dir overrides the env value
 */

#include <ast.h>
#include <ls.h>
#include <error.h>
#include "FEATURE/random"

#define ATTEMPT		10

#define TMP_ENV		"TMPDIR"
#define TMP_PATH_ENV	"TMPPATH"
#define TMP1		"/tmp"
#define TMP2		"/var/tmp"

static inline int xaccess(const char *path, int mode)
{
	static size_t pgsz;
	struct statvfs vfs;
	int ret;

	if (!pgsz)
		pgsz = astconf_ulong(CONF_PAGESIZE);

	if (!path || !*path)
	{
		errno = EFAULT;
		goto err;
	}

	do
		ret = statvfs(path, &vfs);
	while (ret < 0 && errno == EINTR);

	if (ret < 0)
		goto err;

	if (vfs.f_frsize*vfs.f_bavail < pgsz)
	{
		errno = ENOSPC;
		goto err;
	}

	return eaccess(path, mode);
err:
	return -1;
}

#define VALID(d)	(*(d)&&!xaccess(d,W_OK|X_OK))

static struct
{
	char**		vec;
	char**		dir;
	mode_t		mode;
	int		manual;
	char*		pfx;
	char*		tmpdir;
	char*		tmppath;
} tmp = { .mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH };

char*
pathtemp(char* buf, size_t len, const char* dir, const char* pfx, int* fdp)
{
	char*		d;
	char*		b;
	char*		s;
	char*		x;
	uint32_t	num;
	int		n;
	ptrdiff_t	m;
	ptrdiff_t	l;
	ptrdiff_t	r;
	int		z;
	char		numbuf[16];

	if (pfx && *pfx == '/')
	{
		pfx++;
		if (streq(pfx, "cycle"))
		{
			if (!dir)
			{
				tmp.manual = 1;
				if (tmp.dir && !*tmp.dir++)
					tmp.dir = tmp.vec;
			}
			else
				tmp.manual = streq(dir, "manual");
			return (char*)pfx;
		}
		else if (streq(pfx, "prefix"))
		{
			if (tmp.pfx)
				free(tmp.pfx);
			tmp.pfx = dir ? strdup(dir) : NULL;
			return (char*)pfx;
		}
		else if (streq(pfx, "private"))
		{
			tmp.mode = S_IRUSR|S_IWUSR;
			return (char*)pfx;
		}
		else if (streq(pfx, "public"))
		{
			tmp.mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
			return (char*)pfx;
		}
		else if (streq(pfx, TMP_ENV))
		{
			if (tmp.vec)
			{
				free(tmp.vec);
				tmp.vec = 0;
			}
			if (tmp.tmpdir)
				free(tmp.tmpdir);
			tmp.tmpdir = dir ? strdup(dir) : NULL;
			return (char*)pfx;
		}
		else if (streq(pfx, TMP_PATH_ENV))
		{
			if (tmp.vec)
			{
				free(tmp.vec);
				tmp.vec = 0;
			}
			if (tmp.tmppath)
				free(tmp.tmppath);
			tmp.tmppath = dir ? strdup(dir) : NULL;
			return (char*)pfx;
		}
		return NULL;
	}
	if (!(d = (char*)dir) || (*d && xaccess(d, W_OK|X_OK)))
	{
		if (!tmp.vec)
		{
			if ((x = tmp.tmppath) || (x = getenv(TMP_PATH_ENV)))
			{
				n = 2;
				s = x;
				while (s = strchr(s, ':'))
				{
					s++;
					n++;
				}
				if (!(tmp.vec = newof(0, char*, (size_t)n, strlen(x) + 1)))
					return NULL;
				tmp.dir = tmp.vec;
				x = strcpy((char*)(tmp.dir + n), x);
				*tmp.dir++ = x;
				while (x = strchr(x, ':'))
				{
					*x++ = 0;
					if (!VALID(*(tmp.dir - 1)))
						tmp.dir--;
					*tmp.dir++ = x;
				}
				if (!VALID(*(tmp.dir - 1)))
					tmp.dir--;
				*tmp.dir = 0;
			}
			else
			{
				if (((d = tmp.tmpdir) || (d = getenv(TMP_ENV))) && !VALID(d))
					d = 0;
				if (!(tmp.vec = newof(0, char*, 2, d ? (strlen(d) + 1) : 0)))
					return NULL;
				if (d)
					*tmp.vec = strcpy((char*)(tmp.vec + 2), d);
			}
			tmp.dir = tmp.vec;
		}
		if (!(d = *tmp.dir++))
		{
			tmp.dir = tmp.vec;
			d = *tmp.dir++;
		}
		if (!d && (!*(d = astconf("TMP", NULL, NULL)) || xaccess(d, W_OK|X_OK)) && xaccess(d = TMP1, W_OK|X_OK) && xaccess(d = TMP2, W_OK|X_OK))
			return NULL;
	}
	if (!len)
		len = PATH_MAX;
	len--;
	if (!(b = buf) && !(b = newof(0, char, len, 1)))
		return NULL;
	z = 0;
	if (!pfx && !(pfx = tmp.pfx))
		pfx = "ast";
	m = (ptrdiff_t)strlen(pfx);
	if (buf && dir && (buf == (char*)dir && (buf + strlen(buf) + 1) == (char*)pfx || buf == (char*)pfx && !*dir) && !strcmp((char*)pfx + m + 1, "XXXXX"))
	{
		d = (char*)dir;
		m += (ptrdiff_t)strlen(d) + 8;
		len = (size_t)m;
		l = 3;
		r = 3;
	}
	else if (*pfx && pfx[m - 1] == 'X')
	{
		for (l = m; l && pfx[l - 1] == 'X'; l--);
		r = m - l;
		m = l;
		l = r / 2;
		r -= l;
	}
	else if (strchr(pfx, '.'))
	{
		m = 5;
		l = 3;
		r = 3;
	}
	else
	{
		z = '.';
		m = 5;
		l = 2;
		r = 3;
	}
	x = b + len;
	s = b;
	if (d)
	{
		while (s < x && (n = *d++))
			*s++ = (char)n;
		if (s < x && s > b && *(s - 1) != '/')
			*s++ = '/';
	}
	if ((x - s) > m)
		x = s + m;
	while (s < x && (n = *pfx++))
	{
		if (n == '/' || n == '\\' || n == z)
			n = '_';
		*s++ = (char)n;
	}
	*s = 0;
	len -= (size_t)(s - b);
	for (size_t attempt = 0; attempt < ATTEMPT; attempt++)
	{
		/*
		 * generate a pseudorandom name using arc4random(3)
		 */

		num = arc4random();
		sfsprintf(numbuf, sizeof(numbuf), "%07.7.32I*u%07.7.32I*u", sizeof(num), num, sizeof(num), (num >> 16) | ((num & 0xffff) << 16));
		sfsprintf(s, len, "%-.*s%s%-.*s", l, numbuf, z ? "." : "", r, numbuf + sizeof(numbuf) / 2);
		if (fdp)
		{
			if ((n = open(b, O_CREAT|O_RDWR|O_EXCL|O_TEMPORARY, tmp.mode)) >= 0)
			{
				*fdp = n;
				return b;
			}
		}
		else if (access(b, F_OK))
			return b;
	}
	if (!buf)
		free(b);
	return NULL;
}
