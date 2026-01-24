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

/*	Discipline to make an unseekable read stream seekable
**
**	sfraise(f,SFSK_DISCARD,0) discards previous seek data
**	but seeks from current offset on still allowed
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 03/18/1998.
*/

typedef struct _skable_s
{	Sfdisc_t	disc;	/* sfio discipline */
	Sfio_t*		shadow;	/* to shadow data */
	Sfoff_t		discard;/* sfseek(f,-1,SEEK_SET) discarded data */
	Sfoff_t		extent; /* shadow extent */
	int		eof;	/* if eof has been reached */
} Seek_t;

union Seek_u
{
	Seek_t		*seek;
	Sfdisc_t	*disc;
};

static ssize_t skwrite(Sfio_t*		f,	/* stream involved */
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

static ssize_t skread(Sfio_t*	f,	/* stream involved */
		      void*	buf,	/* buffer to read into */
		      size_t	n,	/* number of bytes to read */
		      Sfdisc_t*	disc)	/* discipline */
{
	union Seek_u	sk;
	Sfio_t*		sf;
	Sfoff_t		addr;
	ssize_t		r, w, p;

	sk.disc = disc;
	sf = sk.seek->shadow;
	if(sk.seek->eof)
		return sfread(sf,buf,n);

	addr = sfseek(sf,0,SEEK_CUR);

	if((addr+(ssize_t)n) <= sk.seek->extent)
		return sfread(sf,buf,n);

	if((r = (ssize_t)(sk.seek->extent-addr)) > 0)
	{	if((w = sfread(sf,buf,(size_t)r)) != r)
			return w;
		buf = (char*)buf + r;
		n -= (size_t)r;
	}

	/* do a raw read */
	if((w = sfrd(f,buf,n,disc)) <= 0)
	{	sk.seek->eof = 1;
		w = 0;
	}
	else
	{
		if((p = sfwrite(sf,buf,(size_t)w)) != w)
			sk.seek->eof = 1;
		if(p > 0)
			sk.seek->extent += p;
	}

	return r+w;
}

static Sfoff_t skseek(Sfio_t* f, Sfoff_t addr, int type, Sfdisc_t* disc)
{
	union Seek_u	sk;
	Sfio_t*		sf;
	char		buf[SFIO_BUFSIZE];
	ssize_t		r, w;

	sk.disc = disc;
	sf = sk.seek->shadow;

	switch (type)
	{
	case SEEK_SET:
		addr -= sk.seek->discard;
		break;
	case SEEK_CUR:
		addr += sftell(sf);
		break;
	case SEEK_END:
		addr += sk.seek->extent;
		break;
	default:
		return -1;
	}

	if(addr < 0)
		return (Sfoff_t)(-1);
	else if(addr > sk.seek->extent)
	{	if(sk.seek->eof)
			return (Sfoff_t)(-1);

		/* read enough to reach the seek point */
		while(addr > sk.seek->extent)
		{	if(addr > sk.seek->extent+(ssize_t)sizeof(buf) )
				w = sizeof(buf);
			else	w = (int)(addr-sk.seek->extent);
			if((r = sfrd(f,buf,(size_t)w,disc)) <= 0)
				w = r-1;
			else if((w = sfwrite(sf,buf,(size_t)r)) > 0)
				sk.seek->extent += w;
			if(w != r)
			{	sk.seek->eof = 1;
				break;
			}
		}

		if(addr > sk.seek->extent)
			return (Sfoff_t)(-1);
	}

	return sfseek(sf,addr,SEEK_SET) + sk.seek->discard;
}

/* on close, remove the discipline */
static int skexcept(Sfio_t* f, int type, void* data, Sfdisc_t* disc)
{
	union Seek_u	sk;

	NOT_USED(f);
	NOT_USED(data);
	sk.disc = disc;

	switch (type)
	{
	case SFIO_FINAL:
	case SFIO_DPOP:
		sfclose(sk.seek->shadow);
		free(disc);
		break;
	case SFSK_DISCARD:
		sk.seek->eof = 0;
		sk.seek->discard += sk.seek->extent;
		sk.seek->extent = 0;
		sfseek(sk.seek->shadow,0,SEEK_SET);
		break;
	}
	return 0;
}

int sfdcseekable(Sfio_t* f)
{
	union Seek_u	sk;
	Seek_t		*skk;

	/* see if already seekable */
	if(sfseek(f,0,SEEK_CUR) >= 0)
		return 0;

	if(!(sk.seek = (Seek_t*)malloc(sizeof(Seek_t))) )
		return -1;
	memset(sk.seek, 0, sizeof(*skk));

	sk.seek->disc.readf = skread;
	sk.seek->disc.writef = skwrite;
	sk.seek->disc.seekf = skseek;
	sk.seek->disc.exceptf = skexcept;
	sk.seek->shadow = sftmp(SFIO_BUFSIZE);
	sk.seek->discard = 0;
	sk.seek->extent = 0;
	sk.seek->eof = 0;

	if(sfdisc(f, sk.disc) != sk.disc)
	{	sfclose(sk.seek->shadow);
		free(sk.seek);
		return -1;
	}

	return 0;
}
