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

/*	A discipline to tee the output to a stream to another stream.
**	This is similar to what the "tee" program does. As implemented
**	this discipline only works with file streams.
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 03/18/1998.
*/

/* the discipline structure for tee-ing */
typedef struct _tee_s
{	Sfdisc_t	disc;	/* the sfio discipline structure */
	Sfio_t*		tee;	/* the stream to tee to */
	int		status;	/* if tee stream is still ok */
} Tee_t;

typedef union
{
	Tee_t		*te;
	Sfdisc_t	*disc;
} Tee_u;

/*	write to the teed stream.  */
static ssize_t teewrite(Sfio_t* 	f,	/* the stream being written to */
			const void*	buf,	/* the buffer of data being output */
			size_t		size,	/* the data size */
			Sfdisc_t*	disc)	/* the tee discipline */
{
	Tee_u tu = { .disc = disc };

	/* tee data if still ok */
	if(tu.te->status == 0 && sfwrite(tu.te->tee,buf,size) != (ssize_t)size)
		tu.te->status = -1;

	/* do the actual write */
	return sfwr(f,buf,size,disc);
}

/* on close, remove the discipline */
static int teeexcept(Sfio_t* f, int type, void* data, Sfdisc_t* disc)
{
	NOT_USED(f);
	NOT_USED(data);

	if(type == SFIO_FINAL || type == SFIO_DPOP)
		free(disc);

	return 0;
}

int sfdctee(Sfio_t*	f,	/* stream to tee from	*/
	    Sfio_t*	tee)	/* stream to tee to	*/
{
	Tee_u	tu;

	if(!(tu.te = malloc(sizeof(Tee_t))) )
		return -1;

	tu.te->disc.readf = NULL;
	tu.te->disc.seekf = NULL;
	tu.te->disc.writef = teewrite;
	tu.te->disc.exceptf = teeexcept;
	tu.te->tee = tee;
	tu.te->status = 0;

	if(sfdisc(f,tu.disc) != tu.disc)
	{	free(tu.te);
		return -1;
	}

	return 0;
}
