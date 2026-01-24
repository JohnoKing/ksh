/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
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
#include	"sfdchdr.h"


/*	Make a sequence of streams act like a single stream.
**	This is for reading only.
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 03/18/1998.
*/

#define UNSEEKABLE	1

typedef struct _file_s
{	Sfio_t*	f;	/* the stream		*/
	Sfoff_t	lower;	/* its lowest end	*/
} File_t;

typedef struct _union_s
{
	Sfdisc_t	disc;	/* discipline structure */
	int		type;	/* type of streams	*/
	int		c;	/* current stream	*/
	int		n;	/* number of streams	*/
	Sfoff_t		here;	/* current location	*/
	File_t		f[1];	/* array of streams	*/
} Union_t;

typedef union
{
	Union_t		*un;
	Sfdisc_t	*disc;
} Union_u;

static ssize_t unwrite(Sfio_t*		f,	/* stream involved */
		       const void*	buf,	/* buffer to read into */
		       size_t		n,	/* number of bytes to read */
		       Sfdisc_t*	disc)	/* discipline */
{
	NOT_USED(f);
	NOT_USED(buf);
	NOT_USED(n);
	NOT_USED(disc);
	return -1;
}

static ssize_t unread(Sfio_t*	f,	/* stream involved */
		      void*	buf,	/* buffer to read into */
		      size_t	n,	/* number of bytes to read */
		      Sfdisc_t*	disc)	/* discipline */
{
	Union_u		un;
	ssize_t		r, m;

	un.disc = disc;
	m = (ssize_t)n;
	f = un.un->f[un.un->c].f;
	while(1)
	{	if((r = sfread(f,buf,(size_t)m)) < 0 || (r == 0 && un.un->c == un.un->n-1) )
			break;

		m -= r;
		un.un->here += r;

		if(m == 0)
			break;

		buf = (char*)buf + r;
		if(sfeof(f) && un.un->c < un.un->n-1)
			f = un.un->f[un.un->c += 1].f;
	}
	return (ssize_t)n-m;
}

static Sfoff_t unseek(Sfio_t* f, Sfoff_t addr, int type, Sfdisc_t* disc)
{
	Union_u		un;
	int		i;
	Sfoff_t	extent, s;

	NOT_USED(f);

	un.disc = disc;
	if(un.un->type&UNSEEKABLE)
		return -1L;

	if(type == 2)
	{	extent = 0;
		for(i = 0; i < un.un->n; ++i)
			extent += (sfsize(un.un->f[i].f) - un.un->f[i].lower);
		addr += extent;
	}
	else if(type == 1)
		addr += un.un->here;

	if(addr < 0)
		return -1;

	/* find the stream where the addr could be in */
	extent = 0;
	for(i = 0; i < un.un->n-1; ++i)
	{	s = sfsize(un.un->f[i].f) - un.un->f[i].lower;
		if(addr < extent + s)
			break;
		extent += s;
	}

	s = (addr-extent) + un.un->f[i].lower;
	if(sfseek(un.un->f[i].f,s,0) != s)
		return -1;

	un.un->c = i;
	un.un->here = addr;

	for(i += 1; i < un.un->n; ++i)
		sfseek(un.un->f[i].f,un.un->f[i].lower,0);

	return addr;
}

/* on close, remove the discipline */
static int unexcept(Sfio_t* f, int type, void* data, Sfdisc_t* disc)
{
	NOT_USED(f);
	NOT_USED(data);

	if(type == SFIO_FINAL || type == SFIO_DPOP)
		free(disc);

	return 0;
}

int sfdcunion(Sfio_t* f, Sfio_t** array, int n)
{
	Union_u		un;

	if(n <= 0)
		return -1;

	if(!(un.un = malloc(sizeof(Union_t)+((size_t)n-1)*sizeof(File_t))) )
		return -1;
	memset(un.un, 0, sizeof(*un.un));

	un.un->disc.readf = unread;
	un.un->disc.writef = unwrite;
	un.un->disc.seekf = unseek;
	un.un->disc.exceptf = unexcept;
	un.un->n = n;

	for(int i = 0; i < n; ++i)
	{	un.un->f[i].f = array[i];
		if(!(un.un->type&UNSEEKABLE))
		{	un.un->f[i].lower = sfseek(array[i],0,1);
			if(un.un->f[i].lower < 0)
				un.un->type |= UNSEEKABLE;
		}
	}

	if(sfdisc(f,un.disc) != un.disc)
	{	free(un.un);
		return -1;
	}

	return 0;
}
