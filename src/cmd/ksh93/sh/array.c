/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                  David Korn <dgk@research.att.com>                   *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*         hyenias <58673227+hyenias@users.noreply.github.com>          *
*                                                                      *
***********************************************************************/
/*
 * Array processing routines
 *
 *   David Korn
 *   AT&T Labs
 *   dgk@research.att.com
 *
 */

#include	"shopt.h"
#include	"defs.h"
#include	"name.h"
#include	<ast_release.h>

#if _AST_release
#define NDEBUG
#endif
#include	<assert.h>

#define NUMSIZE	11
#define is_associative(ap)	array_assoc((Namarr_t*)(ap))
#define array_setbit(cp, n, b)	(cp[n] |= (b))
#define array_clrbit(cp, n, b)	(cp[n] &= ~(b))
#define array_isbit(cp, n, b)	(cp[n] & (b))
#define NV_CHILD		NV_EXPORT
#define ARRAY_CHILD		1
#define ARRAY_NOFREE		2

struct index_array
{
	Namarr_t        header;
	void		*xp;		/* if set, subscripts will be converted */
	ptrdiff_t	cur;    	/* index of current element */
	ptrdiff_t	maxi;   	/* maximum index for array */
	unsigned char	*bits;		/* bit array for child subscripts */
	void		*val[1];	/* array of value holders */
};

struct assoc_array
{
	Namarr_t	header;
	Namval_t	*pos;
	Namval_t	*nextpos;
	Namval_t	*cur;
};

typedef union
{
	Namarr_t		*na;
	Namfun_t		*nfp;
	struct index_array	*ip;
} Index_array_u;

typedef union
{
	Namarr_t		*na;
	Namfun_t		*nfp;
	struct assoc_array	*asp;
} Assoc_array_u;

#if SHOPT_FIXEDARRAY
   typedef union
   {
	Namarr_t		*na;
	struct fixed_array	*fp;
   } Fixed_array_u;
   typedef union
   {
	ptrdiff_t		*maxp;
	struct fixed_array	*fp;
   } Fixed_array_max_u;
#  define array_fixed_data(ap)	((ap)?(ap)->fixed->data:NULL)
   static void array_fixed_setdata(Namval_t*,Namarr_t*,struct fixed_array*);
#endif /* SHOPT_FIXEDARRAY */

static Namarr_t *array_scope(Namarr_t *ap, nvflag_t flags)
{
	Namarr_t *aq;
#if SHOPT_FIXEDARRAY
	Fixed_array_u fu;
#endif /* SHOPT_FIXEDARRAY */
	Index_array_u ar;
	size_t size = ap->hdr.dsize;
	if(size==0)
		size = ap->hdr.disc->dsize;
	aq = sh_newof(NULL,Namarr_t,1,size-sizeof(Namarr_t));
	memcpy(aq,ap,size);
	aq->hdr.nofree &= ~1;
	aq->hdr.nofree |= (flags&NV_RDONLY)?1:0;
	if(is_associative(aq))
	{
		aq->scope = dtopen(&_Nvdisc,Dtoset);
		dtview((Dt_t*)aq->scope,aq->table);
		aq->table = (Dt_t*)aq->scope;
		return aq;
	}
#if SHOPT_FIXEDARRAY
	else if(ap->fixed)
	{
		Fixed_array_max_u max_u;
		aq->scope = ap;
		fu.na = aq+1;
		aq->fixed = fu.fp;
		max_u.fp = fu.fp+1;
		fu.fp->max = max_u.maxp;
		fu.fp->incr = fu.fp->max+fu.fp->ndim;
		fu.fp->cur = fu.fp->incr+fu.fp->ndim;
		return aq;
	}
#endif /* SHOPT_FIXEDARRAY */
	aq->scope = ap;
	ar.na = aq;
	memset(ar.ip->val, 0, (size_t)ar.ip->maxi*sizeof(char*));
	ar.ip->bits =  (unsigned char*)&ar.ip->val[ar.ip->maxi];
	return aq;
}

static int array_unscope(Namval_t *np,Namarr_t *ap)
{
	Namfun_t *fp;
	Namunion_u u;
	if(!ap->scope)
		return 0;
	if(is_associative(ap))
		(*ap->fun)(np, NULL, NV_AFREE);
	u.na = ap;
	if((fp = nv_disc(np,u.nfp,NV_POP)) && !(fp->nofree&1))
		free(fp);
	nv_delete(np,NULL,0);
	return 1;
}

static inline void array_syncsub(Namarr_t *ap, Namarr_t *aq)
{
	Index_array_u up, uq;
	up.na = ap;
	uq.na = aq;
	up.ip->cur = uq.ip->cur;
}

static int array_covered(struct index_array *ap)
{
	struct index_array *aq = (struct index_array*)ap->header.scope;
	if(!ap->header.fun && aq)
#if SHOPT_FIXEDARRAY
		return (ap->header.fixed || ((ap->cur<aq->maxi) && aq->val[ap->cur]));
#else
		return ((ap->cur<aq->maxi) && aq->val[ap->cur]);
#endif /* SHOPT_FIXEDARRAY */
	return 0;
}

/*
 * replace discipline with new one
 */
static void array_setptr(Namval_t *np, struct index_array *old, struct index_array *new)
{
	Namfun_t **fp = &np->nvfun;
	while(*fp && *fp!= &old->header.hdr)
		fp = &((*fp)->next);
	if(!*fp)
		abort();
	new->header.hdr.next = (*fp)->next;
	*fp = &new->header.hdr;
}

/*
 *   Calculate the amount of space to be allocated to hold an
 *   indexed array into which <maxi> is a legal index.  The number of
 *   elements that will actually fit into the array (> <maxi>
 *   but <= ARRAY_MAX) is returned.
 *
 */
static ptrdiff_t arsize(struct index_array *ap, ptrdiff_t maxi)
{
	if(ap && maxi < 2*ap->maxi)
		maxi = 2*ap->maxi;
	maxi = roundof(maxi,ARRAY_INCR);
	return maxi>ARRAY_MAX?ARRAY_MAX:maxi;
}

static struct index_array *array_grow(Namval_t*, struct index_array*,ptrdiff_t);

/* return index of highest element of an array */
ptrdiff_t array_maxindex(Namval_t *np)
{
	Index_array_u ap = { .na = nv_arrayptr(np) };
	ptrdiff_t i = ap.ip->maxi;
	if(is_associative(ap.na))
		return -1;
	while(i>0 && !ap.ip->val[--i]);
	return i+1;
}

static void **array_getup(Namval_t *np, Namarr_t *arp, int update)
{
	Index_array_u ap = { .na = arp };
	void **vpp;	/* pointer to value pointer */
#if SHOPT_FIXEDARRAY
	struct fixed_array *fp;
#endif /* SHOPT_FIXEDARRAY */
	int	nofree=0;
	if(!arp)
		return &np->nvalue;
	if(is_associative(ap.na))
	{
		Namval_t	*mp;
		mp = (Namval_t*)((*arp->fun)(np,NULL,NV_ACURRENT));
		if(mp)
		{
			nofree = nv_isattr(mp,NV_NOFREE);
			vpp = &mp->nvalue;
		}
		else
			return (*arp->fun)(np,NULL,0);
	}
#if SHOPT_FIXEDARRAY
	else if(fp = arp->fixed)
	{
		if(!fp->data)
			array_fixed_setdata(np,arp,fp);
		vpp = &np->nvalue;
		if(fp->ptr)
			*vpp = *(((char**)fp->data) + fp->curi);
		else
			*vpp = fp->data + fp->size * (size_t)fp->curi;
	}
#endif /* SHOPT_FIXEDARRAY */
	else
	{
		if(ap.ip->cur >= ap.ip->maxi)
		{
			errormsg(SH_DICT,ERROR_exit(1),e_subscript,nv_name(np));
			UNREACHABLE();
		}
		vpp = &(ap.ip->val[ap.ip->cur]);
		nofree = array_isbit(ap.ip->bits,ap.ip->cur,ARRAY_NOFREE);
	}
	if(update)
	{
		if(nofree)
			nv_onattr(np,NV_NOFREE);
		else
			nv_offattr(np,NV_NOFREE);
	}
	return vpp;
}

int nv_arrayisset(Namval_t *np, Namarr_t *arp)
{
	Index_array_u ap = { .na = arp };
	void *vp; /* value pointer */
	if(is_associative(ap.na))
		return (np = nv_opensub(np)) && !nv_isnull(np);
	if(ap.ip->cur >= ap.ip->maxi)
		return 0;
	vp = ap.ip->val[ap.ip->cur];
	if(vp==Empty)
	{
		Namfun_t *fp = &arp->hdr;
		for(fp=fp->next; fp; fp = fp->next)
		{
			if(fp->disc && (fp->disc->getnum || fp->disc->getval))
				return 1;
		}
	}
	return vp && vp!=Empty;
}

/*
 * Get the value pointer for an array.
 * Delete space as necessary if flag is ARRAY_DELETE
 * After the lookup is done the last @ or * subscript is incremented
 */
static Namval_t *array_find(Namval_t *np,Namarr_t *arp, nvflag_t flag)
{
	Index_array_u 		ap = { .na = arp };
	void			**vpp;	/* pointer to value pointer */
	Namval_t		*mp;
	int			wasundef;
#if SHOPT_FIXEDARRAY
	struct fixed_array	*fp = arp->fixed;
#endif /* SHOPT_FIXEDARRAY */
	if(flag&ARRAY_LOOKUP)
		ap.ip->header.nelem &= ~ARRAY_NOSCOPE;
	else
		ap.ip->header.nelem |= ARRAY_NOSCOPE;
	if(wasundef = ap.ip->header.nelem&ARRAY_UNDEF)
	{
		ap.ip->header.nelem &= ~ARRAY_UNDEF;
		/* delete array is the same as delete array[@] */
		if(flag&ARRAY_DELETE)
		{
#if SHOPT_FIXEDARRAY
			nv_putsub(np, NULL, ARRAY_SCAN|ARRAY_NOSCOPE|(ap.ip->header.fixed?(ARRAY_UNDEF|ARRAY_FIXED):0));
#else
			nv_putsub(np, NULL, ARRAY_SCAN|ARRAY_NOSCOPE);
#endif /* SHOPT_FIXEDARRAY */
			ap.ip->header.nelem |= ARRAY_SCAN;
		}
		else /* same as array[0] */
		{
			if(is_associative(ap.na))
				(*ap.ip->header.fun)(np,"0",flag==ARRAY_ASSIGN?NV_AADD:0);
#if SHOPT_FIXEDARRAY
			else if(fp)
			{
				ptrdiff_t n=(ptrdiff_t)fp->ndim;
				fp->curi = 0;
				while(--n>=0)
					fp->cur[n] = 0;
			}
#endif /* SHOPT_FIXEDARRAY */
			else
				ap.ip->cur = 0;
		}
	}
	if(is_associative(ap.na))
	{
		mp = (Namval_t*)((*arp->fun)(np,NULL,NV_ACURRENT));
		if(!mp)
		{
			Namval_voidp_u vn = { .nv = &mp };
			vpp = vn.vv;
		}
		else if(nv_isarray(mp))
		{
			if(wasundef)
				nv_putsub(mp,NULL,ARRAY_UNDEF);
			return mp;
		}
		else
		{
			vpp = &mp->nvalue;
			if(nv_isvtree(mp))
			{
				if(!*vpp && flag==ARRAY_ASSIGN)
				{
					nv_arraychild(np,mp,0);
					ap.ip->header.nelem++;
				}
				return mp;
			}
		}
	}
#if SHOPT_FIXEDARRAY
	else if(fp)
	{
		char	*data = array_fixed_data((Namarr_t*)ap.ip->header.scope);
		if(flag==ARRAY_ASSIGN && data==fp->data)
		{
			if(data)
			{
				fp->data = (char*)sh_malloc((size_t)fp->nelem*fp->size);
				memcpy(fp->data,data,(size_t)fp->nelem*fp->size);
			}
			else
				array_fixed_setdata(np,&ap.ip->header,fp);
		}
		if(fp->ptr)
		{
			if(!fp->data)
				array_fixed_setdata(np,&ap.ip->header,fp);
			np->nvalue = *(((char**)fp->data)+fp->curi);
		}
		else
			np->nvalue = fp->data+fp->size*(size_t)fp->curi;
		return np;
	}
#endif /* SHOPT_FIXEDARRAY */
	else
	{
		if(!(ap.ip->header.nelem&ARRAY_SCAN) && ap.ip->cur >= ap.ip->maxi)
			ap.ip = array_grow(np, ap.ip, ap.ip->cur);
		if(ap.ip->cur>=ap.ip->maxi)
		{
			errormsg(SH_DICT,ERROR_exit(1),e_subscript,nv_name(np));
			UNREACHABLE();
		}
		vpp = &(ap.ip->val[ap.ip->cur]);
		if((!*vpp || *vpp==Empty) && nv_type(np) && nv_isvtree(np))
		{
			char *cp;
			if(!ap.ip->header.table)
				ap.ip->header.table = dtopen(&_Nvdisc,Dtoset);
			sfprintf(sh.strbuf,"%td",ap.ip->cur);
			cp = sfstruse(sh.strbuf);
			mp = nv_search(cp, ap.ip->header.table, NV_ADD);
			mp->nvmeta = np;
			nv_arraychild(np,mp,0);
		}
		if(*vpp && array_isbit(ap.ip->bits,ap.ip->cur,ARRAY_CHILD))
		{
			if(wasundef && nv_isarray((Namval_t*)*vpp))
				nv_putsub(*vpp,NULL,ARRAY_UNDEF);
			return *vpp;
		}
	}
	np->nvalue = *vpp;
	if(!*vpp)
	{
		Namunion_u nu = { .np = np };
		char *xp = nv_setdisc(np,"get",np,nu.nfp);
		if(flag!=ARRAY_ASSIGN)
			return xp && xp != (char*)np ? np : 0;
		if(!array_covered(ap.ip))
			ap.ip->header.nelem++;
	}
	return np;
}

/*
 * for 'typeset -T' types
 */
int nv_arraysettype(Namval_t *np, Namval_t *tp, const char *sub, nvflag_t flags)
{
	Namval_t	*nq;
	Namarr_t	*ap = nv_arrayptr(np);
	sh.last_table = 0;
	if(!ap->table)
		ap->table = dtopen(&_Nvdisc,Dtoset);
	if(nq = nv_search(sub, ap->table, NV_ADD))
	{
		char	*saved_value = NULL;
		if(!nq->nvfun && nq->nvalue && *((char*)nq->nvalue)==0)
			nv_unset(nq,NV_RDONLY);
		nv_arraychild(np,nq,0);
		if(!nv_isattr(tp,NV_BINARY))
			saved_value = nv_getval(np);
		if(!nv_clone(tp,nq,flags|NV_NOFREE))
			return 0;
		if(!nv_isattr(np,NV_RDONLY))
			nv_offattr(nq,NV_RDONLY);
		if(saved_value)
			nv_putval(nq,saved_value,0);
		ap->nelem |= ARRAY_SCAN;
		return 1;
	}
	return 0;
}


static Namfun_t *array_clone(Namval_t *np, Namval_t *mp, nvflag_t flags, Namfun_t *fp)
{
	Namval_t		*nq, *mq;
	char			*name, *sub=NULL;
	int			skipped=0;
	ptrdiff_t		nelem;
	Namunion_u		ap = { .nfp = fp };
	Index_array_u		iu = { .na = ap.na };
	struct index_array	*aq = iu.ip, *ar;
	Dt_t			*otable = ap.na->table;
	if(flags&NV_MOVE)
	{
		if((flags&NV_COMVAR) && nv_putsub(np,NULL,ARRAY_SCAN))
		{
			do
			{
				if(nq=nv_opensub(np))
					nq->nvmeta = mp;
			}
			while(nv_nextsub(np));
		}
		return fp;
	}
	nelem = ap.na->nelem;
	if(nelem&ARRAY_NOCLONE)
		return NULL;
	if((flags&NV_TYPE) && !ap.na->scope)
	{
		ap.na = array_scope(ap.na,flags);
		return &ap.na->hdr;
	}
	ap.nfp = nv_clone_disc(fp,0);
	if(flags&NV_COMVAR)
	{
		ap.na->scope = 0;
		ap.na->nelem = 0;
		sh.prev_table = sh.last_table;
		sh.prev_root = sh.last_root;
	}
	if(ap.na->table)
	{
		ap.na->table = dtopen(&_Nvdisc,Dtoset);
		if(ap.na->scope && !(flags&NV_COMVAR))
		{
			ap.na->scope = ap.na->table;
			dtview(ap.na->table, otable->view);
		}
	}
	mp->nvfun = ap.nfp;
	mp->nvflag &= NV_MINIMAL;
	mp->nvflag |= (np->nvflag&~(NV_MINIMAL|NV_NOFREE));
	if(!(nelem&(ARRAY_SCAN|ARRAY_UNDEF)) && (sub=nv_getsub(np)))
		sub = sh_strdup(sub);
	iu.na = ap.na;
	ar = iu.ip;
	if(!is_associative(ap.na))
		ar->bits = (unsigned char*)&ar->val[ar->maxi];
	if(!nv_putsub(np,NULL,ARRAY_SCAN|((flags&NV_COMVAR)?0:ARRAY_NOSCOPE)))
	{
		if(ap.na->fun)
			(*ap.na->fun)(np,(char*)np,0);
		skipped=1;
		goto skip;
	}
	do
	{
		name = nv_getsub(np);
		nv_putsub(mp,name,ARRAY_ADD|ARRAY_NOSCOPE);
		mq = 0;
		if(nq=nv_opensub(np))
			mq = nv_search(name,ap.na->table,NV_ADD);
		if(nq && (((flags&NV_COMVAR) && nv_isvtree(nq)) || nv_isarray(nq)))
		{
			mq->nvalue = NULL;
			if(!is_associative(ap.na))
				ar->val[ar->cur] = mq;
			nv_clone(nq,mq,flags);
		}
		else if(flags&NV_ARRAY)
		{
			if((flags&NV_NOFREE) && !is_associative(ap.na))
				array_setbit(aq->bits,aq->cur,ARRAY_NOFREE);
			else if(nq && (flags&NV_NOFREE))
			{
				mq->nvalue = nq->nvalue;
				nv_onattr(nq,NV_NOFREE);
			}
		}
		else if(nv_isattr(np,NV_INTEGER))
		{
			Sfdouble_t d= nv_getnum(np);
			if(!is_associative(ap.na))
				ar->val[ar->cur] = NULL;
			nv_putval(mp,(char*)&d,NV_LDOUBLE);
		}
		else
		{
			if(!is_associative(ap.na))
				ar->val[ar->cur] = NULL;
			nv_putval(mp,nv_getval(np),NV_RDONLY);
		}
		aq->header.nelem |= ARRAY_NOSCOPE;
	}
	while(nv_nextsub(np));
skip:
	if(sub)
	{
		if(!skipped)
			nv_putsub(np,sub,0L);
		free(sub);
	}
	aq->header.nelem = ap.na->nelem = nelem;
	return &ap.na->hdr;
}

static char *array_getval(Namval_t *np, Namfun_t *disc)
{
	Namarr_t	*aq;
	Namunion_u	ap = { .nfp = disc };
	Namval_t	*mp;
	char		*cp=NULL;
	if((mp=array_find(np,ap.na,ARRAY_LOOKUP))!=np)
	{
		if(!mp && !is_associative(ap.na) && (aq=(Namarr_t*)ap.na->scope))
		{
			array_syncsub(aq,ap.na);
			if((mp=array_find(np,aq,ARRAY_LOOKUP))==np)
				return nv_getv(np,&aq->hdr);
		}
		if(mp)
		{
			cp = nv_getval(mp);
			nv_offattr(mp,NV_EXPORT);
		}
		return cp;
	}
	return nv_getv(np,&ap.na->hdr);
}

static Sfdouble_t array_getnum(Namval_t *np, Namfun_t *disc)
{
	Namarr_t *aq;
	Namunion_u ap = { .nfp = disc };
	Namval_t *mp;
	if((mp=array_find(np,ap.na,ARRAY_LOOKUP))!=np)
	{
		if(!mp && !is_associative(ap.na) && (aq=(Namarr_t*)ap.na->scope))
		{
			array_syncsub(aq,ap.na);
			if((mp=array_find(np,aq,ARRAY_LOOKUP))==np)
				return nv_getn(np,&aq->hdr);
		}
		return mp ? nv_getnum(mp) : 0;
	}
	return nv_getn(np,&ap.na->hdr);
}

static void array_putval(Namval_t *np, const char *string, nvflag_t flags, Namfun_t *dp)
{
	Namunion_u	ap = { .nfp = dp };
	void		**vpp;	/* pointer to value pointer */
	Namval_t	*mp;
	Index_array_u	aq = { .na = ap.na };
	int		scan,nofree = nv_isattr(np,NV_NOFREE);
#if SHOPT_FIXEDARRAY
	struct fixed_array	*fp;
#endif /* SHOPT_FIXEDARRAY */
	do
	{
		int xfree = (ap.na->fixed||is_associative(ap.na))?0:array_isbit(aq.ip->bits,aq.ip->cur,ARRAY_NOFREE);
		mp = array_find(np,ap.na,string?ARRAY_ASSIGN:ARRAY_DELETE);
		scan = ap.na->nelem&ARRAY_SCAN;
		if(mp && mp!=np)
		{
			if(!is_associative(ap.na) && string && !(flags&NV_APPEND) && !nv_type(np) && nv_isvtree(mp) && !(ap.na->nelem&ARRAY_TREE))
			{
				if(!nv_isattr(np,NV_NOFREE))
					nv_unset(mp,flags&NV_RDONLY);
				array_clrbit(aq.ip->bits,aq.ip->cur,ARRAY_CHILD);
				aq.ip->val[aq.ip->cur] = NULL;
				if(!nv_isattr(mp,NV_NOFREE))
					nv_delete(mp,ap.na->table,0);
				goto skip;
			}
			if(!xfree)
				nv_putval(mp, string, flags);
			if(string)
			{
				if(ap.na->hdr.type && ap.na->hdr.type!=nv_type(mp))
					nv_arraysettype(np,ap.na->hdr.type,nv_getsub(np),0);
				continue;
			}
			ap.na->nelem |= scan;
		}
		if(!string)
		{
			if(mp)
			{
				if(is_associative(ap.na))
				{
					(*ap.na->fun)(np,NULL,NV_ADELETE);
					np->nvalue = NULL;
				}
				else
				{
					Index_array_u iiu;
					if(mp!=np)
					{
						array_clrbit(aq.ip->bits,aq.ip->cur,ARRAY_CHILD);
						aq.ip->val[aq.ip->cur] = NULL;
						if(!xfree)
							nv_delete(mp,ap.na->table,0);
					}
					iiu.na = ap.na;
					if(!array_covered(iiu.ip))
					{
						if(array_elem(ap.na))
							ap.na->nelem--;
					}
#if SHOPT_FIXEDARRAY
					else if(fp=ap.na->fixed)
					{
						char *data = array_fixed_data((Namarr_t*)ap.na->scope);
						size_t n = fp->size*(size_t)fp->curi;
						if(data)
						{
							memcpy(fp->data+n,data+n,fp->size);
							continue;
						}
					}
#endif /* SHOPT_FIXEDARRAY */
				}
			}
			if(array_elem(ap.na)==0 && (ap.na->nelem&ARRAY_SCAN))
			{
				if(is_associative(ap.na))
					(*ap.na->fun)(np, NULL, NV_AFREE);
				else if(ap.na->table && (!sh.subshell || sh.subshare))
				{
					dtclose(ap.na->table);
					ap.na->table = NULL;
				}
				nv_offattr(np,NV_ARRAY);
			}
			if(!mp || mp!=np || is_associative(ap.na))
				continue;
		}
	skip:
		/* prevent empty string from being deleted */
		vpp = array_getup(np,ap.na,!nofree);
		if(*vpp == Empty)
			*vpp = NULL;
#if SHOPT_FIXEDARRAY
		if(nv_isarray(np) && !ap.na->fixed)
#else
		if(nv_isarray(np))
#endif /* SHOPT_FIXEDARRAY */
			np->nvalue = vpp;
		nv_putv(np,string,flags,&ap.na->hdr);
		if(nofree && !*vpp)
			*vpp = Empty;
#if SHOPT_FIXEDARRAY
		if(fp = ap.na->fixed)
		{
			if(fp->ptr)
			{
				char **cp = (char**)fp->data;
				cp[fp->curi] = (char*)(np->nvalue?np->nvalue:Empty);
			}
		}
		else
#endif /* SHOPT_FIXEDARRAY */
		if(!is_associative(ap.na))
		{
			if(string)
				array_clrbit(aq.ip->bits,aq.ip->cur,ARRAY_NOFREE);
			else if(mp==np)
				aq.ip->val[aq.ip->cur] = NULL;
		}
		if(string && ap.na->hdr.type && nv_isvtree(np))
			nv_arraysettype(np,ap.na->hdr.type,nv_getsub(np),0);
	}
	while(!string && nv_nextsub(np));
	if(ap.na)
		ap.na->nelem &= ~ARRAY_NOSCOPE;
	if(nofree)
		nv_onattr(np,NV_NOFREE);
	else
		nv_offattr(np,NV_NOFREE);
	if(!string && !nv_isattr(np,NV_ARRAY))
	{
		Namfun_t *nfp;
#if SHOPT_FIXEDARRAY
		char	*data = array_fixed_data((Namarr_t*)ap.na->scope);
		fp = ap.na->fixed;
		if(fp && (!ap.na->scope || data!=fp->data))
		{
			if(fp->ptr)
			{
				ptrdiff_t n = fp->nelem;
				char **cp = (char**)fp->data;
				while(n-->0)
				{
					if(cp && *cp!=Empty)
						free(*cp);
					cp++;
				}
			}
			free(fp->data);
			if(data)
				fp->data = data;
		}
		else
#endif /* SHOPT_FIXEDARRAY */
		if(!is_associative(ap.na) && aq.ip->xp)
		{
			nv_unset(nv_namptr(aq.ip->xp,0),NV_RDONLY);
			free(aq.ip->xp);
		}
		if((nfp = nv_disc(np,ap.nfp,NV_POP)) && !(nfp->nofree&1))
		{
			ap.na = NULL;
			free(nfp);
		}
		if(!nv_isnull(np))
		{
			nv_onattr(np,NV_NOFREE);
			nv_unset(np,flags);
		}
		else
			nv_offattr(np,NV_NOFREE);
		if(np->nvalue==Empty)
			np->nvalue = NULL;
	}
	if(!string && (flags&NV_TYPE) && ap.na)
		array_unscope(np,ap.na);
}

static const Namdisc_t array_disc =
{
	sizeof(Namarr_t),
	array_putval,
	array_getval,
	array_getnum,
	0,
	0,
	array_clone
};

static void array_copytree(Namval_t *np, Namval_t *mp)
{
	Namfun_t	*fp = nv_disc(np,NULL,NV_POP);
	nv_offattr(np,NV_ARRAY);
	nv_clone(np,mp,0);
	if(!nv_isattr(np,NV_NOFREE))
		free(np->nvalue);
	np->nvalue = &mp->nvalue;
	fp->nofree  &= ~1;
	nv_disc(np,(Namfun_t*)fp, NV_FIRST);
	fp->nofree |= 1;
	nv_onattr(np,NV_ARRAY);
	mp->nvmeta = np;
}

/*
 *        Increase the size of the indexed array of elements in <arp>
 *        so that <maxi> is a legal index.  If <arp> is 0, an array
 *        of the required size is allocated.  A pointer to the
 *        allocated Namarr_t structure is returned.
 *        <maxi> becomes the current index of the array.
 */
static struct index_array *array_grow(Namval_t *np, struct index_array *arp,ptrdiff_t maxi)
{
	Index_array_u ap;
	ptrdiff_t i, newsize = arsize(arp,maxi+1);
	if (maxi >= ARRAY_MAX)
	{
		errormsg(SH_DICT,ERROR_exit(1),e_subscript,fmtint(maxi,1));
		UNREACHABLE();
	}
	i = (newsize - 1) * (ptrdiff_t)sizeof(void*) + newsize;
	ap.ip = new_of(struct index_array,(size_t)i);
	memset(ap.ip,0,sizeof(*ap.ip)+(size_t)i);
	ap.ip->maxi = newsize;
	ap.ip->cur = maxi;
	ap.ip->bits =  (unsigned char*)&ap.ip->val[newsize];
	memset(ap.ip->bits, 0, (size_t)newsize);
	if(arp)
	{
		ap.ip->header = arp->header;
		ap.ip->header.hdr.dsize = sizeof(*ap.ip) + (size_t)i;
		for(i=0;i < arp->maxi;i++)
		{
			ap.ip->bits[i] = arp->bits[i];
			ap.ip->val[i] = arp->val[i];
		}
		memcpy(ap.ip->bits, arp->bits, (size_t)arp->maxi);
		array_setptr(np,arp,ap.ip);
		free(arp);
	}
	else
	{
		Namval_t *mp = NULL;
		ap.ip->header.hdr.dsize = sizeof(*ap.ip) + (size_t)i;
		i = 0;
		ap.ip->header.fun = 0;
		if((nv_isnull(np)||np->nvalue==Empty) && nv_isattr(np,NV_NOFREE))
		{
			i = ARRAY_TREE;
			nv_offattr(np,NV_NOFREE);
		}
		if(np->nvalue==Empty)
			np->nvalue = NULL;
		if(nv_hasdisc(np,&array_disc) || (nv_type(np) && nv_isvtree(np)))
		{
			ap.ip->header.table = dtopen(&_Nvdisc,Dtoset);
			mp = nv_search("0", ap.ip->header.table,NV_ADD);
			if(mp && nv_isnull(mp))
			{
				Namfun_t *fp;
				ap.ip->val[0] = mp;
				array_setbit(ap.ip->bits,0,ARRAY_CHILD);
				for(fp=np->nvfun; fp && !fp->disc->readf; fp=fp->next);
				if(fp && fp->disc && fp->disc->readf)
					(*fp->disc->readf)(mp,NULL,0,fp);
				i++;
			}
		}
		else
		if((ap.ip->val[0] = np->nvalue) || (nv_isattr(np,NV_INTEGER) && !nv_isnull(np)))
			i++;
		ap.ip->header.nelem = i;
		ap.ip->header.hdr.disc = &array_disc;
		nv_disc(np,ap.nfp,NV_FIRST);
		nv_onattr(np,NV_ARRAY);
		if(mp)
		{
			array_copytree(np,mp);
			ap.ip->header.hdr.nofree &= ~1;
		}
	}
	for(;i < newsize;i++)
		ap.ip->val[i] = NULL;
	return ap.ip;
}

int nv_atypeindex(Namval_t *np, const char *tname)
{
	Namval_t	*tp;
	size_t		n = strlen(tname)-1;
	sfprintf(sh.strbuf,"%s.%.*s",NV_CLASS,n,tname);
	tp = nv_open(sfstruse(sh.strbuf), sh.var_tree, NV_NOADD|NV_VARNAME|NV_NOFAIL);
	if(tp)
	{
		Index_array_u ap = { .na = nv_arrayptr(np) };
		if(!nv_hasdisc(tp,&ENUM_disc))
		{
			errormsg(SH_DICT,ERROR_exit(1),e_notenum,tp->nvname);
			UNREACHABLE();
		}
		if(!ap.ip)
			ap.ip = array_grow(np,ap.ip,1);
		ap.ip->xp = sh_calloc(NV_MINSZ,1);
		np = nv_namptr(ap.ip->xp,0);
		np->nvname = tp->nvname;
		nv_onattr(np,NV_MINIMAL);
		nv_clone(tp,np,NV_NOFREE);
		nv_offattr(np,NV_RDONLY);
		return 1;
	}
	errormsg(SH_DICT,ERROR_exit(1),e_unknowntype, n,tname);
	UNREACHABLE();
}

Namarr_t *nv_arrayptr(Namval_t *np)
{
	if(nv_isattr(np,NV_ARRAY))
	{
		Namunion_u u = { .nfp = nv_hasdisc(np, &array_disc) };
		return u.na;
	}
	return NULL;
}

/*
 * Verify that argument is an indexed array and convert to associative,
 * freeing relevant storage
 */
static Namarr_t *nv_changearray(Namval_t *np, void *(*fun)(Namval_t*,const char*,nvflag_t))
{
	Namarr_t *ap;
	char numbuff[NUMSIZE+1];
	unsigned digit, n;
	void **vpp;	/* pointer to value pointer */
	char *string_index=&numbuff[NUMSIZE];
	numbuff[NUMSIZE]='\0';
	Index_array_u save_ap;

	if(!fun || !(ap = nv_arrayptr(np)) || is_associative(ap))
		return NULL;

	nv_stack(np, &ap->hdr);
	save_ap.nfp = nv_stack(np, NULL);
	ap = (Namarr_t*)((*fun)(np, NULL, NV_AINIT));
	ap->nelem = 0;
	ap->fun = fun;
	nv_onattr(np,NV_ARRAY);

	for(unsigned dot = 0; dot < (unsigned)save_ap.ip->maxi; dot++)
	{
		if(save_ap.ip->val[dot])
		{
			if ((digit = dot)== 0)
				*--string_index = '0';
			else while( n = digit )
			{
				digit /= 10;
				*--string_index = (char)('0' + (n-10*digit));
			}
			nv_putsub(np, string_index, ARRAY_ADD);
			vpp = (void**)((*ap->fun)(np,NULL,0));
			*vpp = save_ap.ip->val[dot];
			save_ap.ip->val[dot] = NULL;
		}
		string_index = &numbuff[NUMSIZE];
	}
	free(save_ap.ip);
	return ap;
}

/*
 * set the associative array processing method for node <np> to <fun>
 * The array pointer is returned if successful.
 */
Namarr_t *nv_setarray(Namval_t *np, void *(*fun)(Namval_t*,const char*,nvflag_t))
{
	Namarr_t	*ap;
	char		*value=0;
	Namfun_t	*fp;
	ptrdiff_t	nelem = 0;
	if(fun && (ap = nv_arrayptr(np)))
	{
		/*
		 * if it's already an indexed array, convert to
		 * associative structure
		 */
		if(!is_associative(ap))
			ap = nv_changearray(np, fun);
		return ap;
	}
	if(nv_isnull(np) && nv_isattr(np,NV_NOFREE))
	{
		nelem = ARRAY_TREE;
		nv_offattr(np,NV_NOFREE);
	}
	if(!(fp=nv_isvtree(np)))
		value = nv_getval(np);
	if(fun && !ap && (ap = (Namarr_t*)((*fun)(np, NULL, NV_AINIT))))
	{
		/* check for preexisting initialization and save */
		ap->nelem = nelem;
		ap->fun = fun;
		nv_onattr(np,NV_ARRAY);
		if(fp || (value && value!=Empty))
		{
			nv_putsub(np, "0", ARRAY_ADD);
			if(value)
				nv_putval(np, value, 0);
			else
			{
				Namval_t *mp = (Namval_t*)((*fun)(np,NULL,NV_ACURRENT));
				array_copytree(np,mp);
			}
		}
		return ap;
	}
	return NULL;
}

/*
 * move parent subscript into child
 */
Namval_t *nv_arraychild(Namval_t *np, Namval_t *nq, int c)
{
	Namfun_t		*fp;
	Namarr_t		*ap = nv_arrayptr(np);
	void			**vpp;	/* pointer to value pointer */
	Namval_t		*tp;
	if(!nq)
		return ap ? array_find(np,ap, ARRAY_LOOKUP) : 0;
	if(!ap)
	{
		nv_putsub(np, NULL, ARRAY_FILL);
		ap = nv_arrayptr(np);
	}
	if(!(vpp = array_getup(np,ap,0)))
		return NULL;
	np->nvalue = *vpp;
	if((tp=nv_type(np)) || c)
	{
		ap->nelem |= ARRAY_NOCLONE;
		nq->nvmeta = np;
		if(c=='t')
			nv_clone(tp,nq, 0);
		else
			nv_clone(np, nq, NV_NODISC);
		nv_offattr(nq,NV_ARRAY);
		ap->nelem &= ~ARRAY_NOCLONE;
	}
	nq->nvmeta = np;
	if((fp=nq->nvfun) && fp->disc && fp->disc->setdisc && (fp = nv_disc(nq,fp,NV_POP)))
		free(fp);
	if(!ap->fun)
	{
		Index_array_u aq = { .na = ap };
		array_setbit(aq.ip->bits,aq.ip->cur,ARRAY_CHILD);
		if(c=='.' && !nq->nvalue)
			ap->nelem++;
		*vpp = nq;
	}
	if(c=='.')
		nv_setvtree(nq);
	return nq;
}

/*
 * This routine sets subscript of <np> to the next element, if any.
 * The return value is zero, if there are no more elements
 * Otherwise, 1 is returned.
 */
int nv_nextsub(Namval_t *np)
{
	Index_array_u		ap = { .na = nv_arrayptr(np) };
	size_t			dot;
	struct index_array	*aq=NULL, *ar=NULL;
#if SHOPT_FIXEDARRAY
	struct fixed_array	*fp;
#endif /* SHOPT_FIXEDARRAY */
	if(!ap.ip || !(ap.ip->header.nelem&ARRAY_SCAN))
		return 0;
	if(is_associative(ap.na))
	{
		if((*ap.ip->header.fun)(np,NULL,NV_ANEXT))
			return 1;
		ap.ip->header.nelem &= ~(ARRAY_SCAN|ARRAY_NOCHILD);
		return 0;
	}
#if SHOPT_FIXEDARRAY
	else if(fp = ap.ip->header.fixed)
	{
		if(ap.ip->header.nelem&ARRAY_FIXED)
		{
			while(++fp->curi < fp->nelem)
			{
				nv_putsub(np,0,fp->curi|ARRAY_FIXED|ARRAY_SCAN);
				if(fp->ptr && *(((char**)fp->data)+fp->curi))
					return 1;
			}
			ap.ip->header.nelem &= ~ARRAY_FIXED;
			return 0;
		}
		dot = fp->dim;
		if((fp->cur[dot]+1) < fp->max[dot])
		{
			fp->cur[dot]++;
			for(fp->curi=0,dot=0; dot < fp->ndim; dot++)
				fp->curi +=  fp->incr[dot]*fp->cur[dot];
			return 1;
		}
		if(fp->level)
		{
			dot= --fp->dim;
			while((dot+1) < fp->ndim)
				fp->cur[++dot] = 0;
			fp->level--;
			fp->curi = 0;
		}
		else
		ap.ip->header.nelem &= ~(ARRAY_SCAN|ARRAY_NOCHILD);
		return 0;
	}
#endif /* SHOPT_FIXEDARRAY */
	if(!(ap.ip->header.nelem&ARRAY_NOSCOPE))
		ar = (struct index_array*)ap.ip->header.scope;
	for(dot=(size_t)ap.ip->cur+1; dot < (size_t)ap.ip->maxi; dot++)
	{
		aq = ap.ip;
		if(!ap.ip->val[dot] && !(ap.ip->header.nelem&ARRAY_NOSCOPE))
		{
			if(!(aq=ar) || dot>=(size_t)aq->maxi)
				continue;
		}
		if(aq->val[dot]==Empty && array_elem(&aq->header) < nv_aimax(np)+1)
		{
			ap.ip->cur = (ptrdiff_t)dot;
			if(nv_getval(np)==Empty)
				continue;
		}
		if(aq->val[dot])
		{
			ap.ip->cur = (ptrdiff_t)dot;
			if(array_isbit(aq->bits, dot,ARRAY_CHILD))
			{
				Namval_t *mp = aq->val[dot];
				if((aq->header.nelem&ARRAY_NOCHILD) && nv_isvtree(mp) && !mp->nvfun->dsize)
					continue;
				if(nv_isarray(mp))
					nv_putsub(mp,NULL,ARRAY_SCAN);
			}
			return 1;
		}
	}
	ap.ip->header.nelem &= ~(ARRAY_SCAN|ARRAY_NOCHILD);
	ap.ip->cur = 0;
	return 0;
}

/*
 * Set an array subscript for node <np> given the subscript <sp>
 * An array is created if necessary.
 * <mode> can be a number, plus or more of symbolic constants
 *    ARRAY_SCAN, ARRAY_UNDEF, ARRAY_ADD
 * The node pointer is returned which can be NULL if <np> is
 *    not already array and the ARRAY_ADD bit of <mode> is not set.
 * ARRAY_FILL sets the specified subscript to the empty string when
 *   ARRAY_ADD is specified and there is no value or sets all
 * the elements up to the number specified if ARRAY_ADD is not specified
 */
Namval_t *nv_putsub(Namval_t *np,char *sp,long mode)
{
	Index_array_u ap = { .na = nv_arrayptr(np) };
	ptrdiff_t size = (mode&ARRAY_MASK);
#if SHOPT_FIXEDARRAY
	struct fixed_array	*fp;
	if(!ap.ip || (!ap.ip->header.fixed && !ap.ip->header.fun))
#else
	if(!ap.ip || !ap.ip->header.fun)
#endif /* SHOPT_FIXEDARRAY */
	{
		if(sp && sp!=Empty)
		{
			if(ap.ip && ap.ip->xp && !strmatch(sp,"+([0-9])"))
			{
				Namval_t *mp = nv_namptr(ap.ip->xp,0);
				nv_putval(mp, sp,0);
				size = nv_getnum(mp);
			}
			else
			{
				Dt_t *root = sh.last_root;
				sh.nv_putsub_idx = size = (int)sh_arith((char*)sp);
				sh.nv_putsub_already_called_sh_arith = 1;  /* tell nv_create() to avoid double arith eval */
				sh.last_root = root;
			}
		}
		if(size <0 && ap.ip)
			size += array_maxindex(np);
		if(size >= ARRAY_MAX || (size < 0))
		{
			errormsg(SH_DICT,ERROR_exit(1),e_subscript, nv_name(np));
			UNREACHABLE();
		}
		if(!ap.ip || size>=ap.ip->maxi)
		{
			if(size==0 && !(mode&ARRAY_FILL))
				return NULL;
			if(sh.subshell)
				sh_assignok(np,1);
			ap.ip = array_grow(np,ap.ip,size);
		}
		ap.ip->header.nelem &= ~ARRAY_UNDEF;
		ap.ip->header.nelem |= (mode&(ARRAY_SCAN|ARRAY_NOCHILD|ARRAY_UNDEF|ARRAY_NOSCOPE));
		ap.ip->cur = size;
		if((mode&ARRAY_SCAN) && (ap.ip->cur--,!nv_nextsub(np)))
			np = 0;
		if(mode&(ARRAY_FILL|ARRAY_ADD))
		{
			if(!(mode&ARRAY_ADD))
			{
				if(mode&ARRAY_SETSUB)
				{
					for(ptrdiff_t n=0; n <= ap.ip->maxi; n++)
						ap.ip->val[n] = NULL;
					ap.ip->header.nelem = 0;
				}
				for(ptrdiff_t n=0; n <= size; n++)
				{
					if(!ap.ip->val[n])
					{
						ap.ip->val[n] = Empty;
						if(!array_covered(ap.ip))
							ap.ip->header.nelem++;
					}
				}
			}
			else if(!(sp = ap.ip->val[size]) || sp==Empty)
			{
				if(sh.subshell)
					sh_assignok(np,1);
				if(ap.ip->header.nelem&ARRAY_TREE)
				{
					char *cp;
					Namval_t *mp;
					if(!ap.ip->header.table)
						ap.ip->header.table = dtopen(&_Nvdisc,Dtoset);
					sfprintf(sh.strbuf,"%td",ap.ip->cur);
					cp = sfstruse(sh.strbuf);
					mp = nv_search(cp, ap.ip->header.table, NV_ADD);
					mp->nvmeta = np;
					nv_arraychild(np,mp,0);
					nv_setvtree(mp);
				}
				else if(!sh.cond_expan)
					ap.ip->val[size] = Empty;
				if(!sp && !array_covered(ap.ip))
					ap.ip->header.nelem++;
			}
		}
		else if(!(mode&ARRAY_SCAN))
		{
			ap.ip->header.nelem &= ~ARRAY_SCAN;
			if(array_isbit(ap.ip->bits,size,ARRAY_CHILD))
				nv_putsub(ap.ip->val[size],NULL,ARRAY_UNDEF);
			if(sp && !(mode&ARRAY_ADD) && !ap.ip->val[size])
				np = 0;
		}
		return (Namval_t*)np;
	}
#if SHOPT_FIXEDARRAY
	if(fp=ap.ip->header.fixed)
	{
		if(!fp->data)
			return np;
		if(mode&ARRAY_UNDEF)
		{
			fp->dim = 0;
			fp->curi = 0;
			for(size=(ptrdiff_t)fp->ndim;--size>=0;)
				fp->cur[size] = 0;
			ap.ip->header.nelem &= ~ARRAY_MASK;
			if(mode&ARRAY_FIXED)
			{
				mode &= ~ARRAY_UNDEF;
				ap.ip->header.nelem |= (ARRAY_FIXED|fp->nelem);
			}
			else
				ap.ip->header.nelem |=  fp->max[0];
		}
		else if(mode&ARRAY_FIXED)
		{
			size = (mode&ARRAY_MASK)&~(ARRAY_FIXED);
			fp->curi = size;
			for(fp->dim=0;size>0 && fp->dim<fp->ndim; fp->dim++)
			{
				fp->cur[fp->dim] = size/fp->incr[fp->dim];
				size -= fp->incr[fp->dim]*fp->cur[fp->dim];
			}
			while(fp->dim < fp->ndim)
				fp->cur[fp->dim++] = 0;
			fp->dim = (size_t)ap.ip->header.nelem;
			ap.ip->header.nelem |= ARRAY_FIXED;
		}
		else if(fp->dim< fp->ndim)
		{
			fp->curi += (size-fp->cur[fp->dim])*fp->incr[fp->dim];
			fp->cur[fp->dim] = size;
		}
	}
#endif /* SHOPT_FIXEDARRAY */
	ap.ip->header.nelem &= ~ARRAY_UNDEF;
	if(!(mode&ARRAY_FILL))
		ap.ip->header.nelem &= ~ARRAY_SCAN;
	ap.ip->header.nelem |= (mode&(ARRAY_SCAN|ARRAY_NOCHILD|ARRAY_UNDEF|ARRAY_NOSCOPE));
#if SHOPT_FIXEDARRAY
	if(fp)
		return np;
	else
#endif /* SHOPT_FIXEDARRAY */
	if(sp)
	{
		if(mode&ARRAY_SETSUB)
		{
			(*ap.ip->header.fun)(np, sp, NV_ASETSUB);
			return np;
		}
		(*ap.ip->header.fun)(np, sp, (mode&ARRAY_ADD)?NV_AADD:0);
		if(!(mode&(ARRAY_SCAN|ARRAY_ADD)) && !(*ap.ip->header.fun)(np,NULL,NV_ACURRENT))
			np = 0;
	}
	else if(mode&ARRAY_SCAN)
		(*ap.ip->header.fun)(np,(char*)np,0);
	else if(mode&ARRAY_UNDEF)
		(*ap.ip->header.fun)(np, "",0);
	if((mode&ARRAY_SCAN) && !nv_nextsub(np))
		np = 0;
	return np;
}

#if SHOPT_FIXEDARRAY
ptrdiff_t nv_arrfixed(Namval_t *np, Sfio_t *out, int flag, char *dim)
{
	Namarr_t		*ap = nv_arrayptr(np);
	struct fixed_array	*fp = ap->fixed;
	if(flag)
	{
		if(out)
		{
			for(size_t n=0; n < fp->dim; n++)
				sfprintf(out,"[%td]",fp->cur[n]);
		}
		if(dim)
			*dim = (char)fp->dim;
		return fp->curi;
	}
	if(out)
	{
		for(size_t n=0; n < fp->ndim; n++)
			sfprintf(out,"[%td]",fp->max[n]);
	}
	fp->dim = 0;
	return fp->curi;
}

static void array_fixed_setdata(Namval_t *np,Namarr_t* ap,struct fixed_array* fp)
{
	ptrdiff_t n = ap->nelem;
	ap->nelem = 1;
	fp->size = fp->ptr?sizeof(void*):nv_datasize(np,0);
	ap->nelem = n;
	fp->data = (char*)sh_calloc((size_t)fp->nelem,fp->size);
	if(fp->ptr)
	{
		char **cp = (char**)fp->data;
		for(n=fp->nelem; n-->0;)
			*cp++ = Empty;
	}
}

static int array_fixed_init(Namval_t *np, char *sub, char *cp)
{
	Namunion_u		ap;
	Fixed_array_u		fu;
	ptrdiff_t		n=1,sz;
	char			*ep=cp;
	Fixed_array_max_u	max_u;
	while(*ep=='[')
	{
		ep = nv_endsubscript(np,ep,0);
		n++;
	}
	if(*ep)
		return 0;
	sz = (ptrdiff_t)sizeof(struct fixed_array)+ 3*n*(ptrdiff_t)sizeof(ptrdiff_t);
	ap.na = sh_newof(NULL,Namarr_t,1,(size_t)sz);
	ap.na->hdr.disc = &array_disc;
	ap.na->hdr.dsize = sizeof(Namarr_t)+(size_t)sz;
	ap.na->hdr.nofree &= ~1;
	fu.na = ap.na+1;
	ap.na->fixed = fu.fp;
	fu.fp->ndim = (size_t)n;
	max_u.fp = fu.fp+1;
	fu.fp->max = max_u.maxp;
	fu.fp->incr = fu.fp->max+n;
	fu.fp->cur = fu.fp->incr+n;
	fu.fp->max[0] = (ptrdiff_t)sh_arith((char*)sub);
	for(n=1,ep=cp;*ep=='['; ep=cp)
	{
		cp = nv_endsubscript(np,ep,0);
		cp[-1]=0;
		fu.fp->max[n++] = sz = (ptrdiff_t)sh_arith((char*)ep+1);
		if(sz<0)
		{
			free(ap.na);
			errormsg(SH_DICT,ERROR_exit(1),e_subscript, nv_name(np));
			UNREACHABLE();
		}
		cp[-1] = ']';
	}
	nv_disc(np,ap.nfp, NV_FIRST);
	fu.fp->ptr = !np->nvsize;
	nv_onattr(np,NV_ARRAY|(fu.fp->ptr?0:NV_NOFREE));
	fu.fp->incr[n=(ptrdiff_t)fu.fp->ndim-1] = 1;
	for(sz=1; --n>=0;)
		sz = fu.fp->incr[n] = sz*fu.fp->max[n+1];
	fu.fp->nelem = sz*fu.fp->max[0];
	ap.na->nelem = fu.fp->max[0];
	return 1;
}

static char *array_fixed(Namval_t *np, char *sub, char *cp)
{
	Namarr_t		*ap = nv_arrayptr(np);
	struct fixed_array	*fp = ap->fixed;
	char			*ep;
	ptrdiff_t		size,n=0,sz;
	if(!fp->data)
		array_fixed_setdata(np,ap,fp);
	ap->nelem &= ~ARRAY_UNDEF;
	if(ap->nelem&ARRAY_FIXED)
	{
		ap->nelem &= ~ARRAY_FIXED;
		n = (ptrdiff_t)fp->dim;
		sz = fp->curi;
		if(*sub==0)
			goto skip;
	}
	else
		fp->curi = 0;
	size = (ptrdiff_t)sh_arith((char*)sub);
	fp->cur[n] = size;
	if(size >= fp->max[n] || (size < 0))
	{
		errormsg(SH_DICT,ERROR_exit(1),e_subscript, nv_name(np));
		UNREACHABLE();
	}
	*cp++ = ']';
	sz = fp->curi + fp->cur[n]*fp->incr[n];
	for(n++,ep=cp;*ep=='['; ep=cp,n++)
	{
		if(n >= (ptrdiff_t)fp->ndim)
		{
			errormsg(SH_DICT,ERROR_exit(1),e_subscript, nv_name(np));
			UNREACHABLE();
		}
		cp = nv_endsubscript(np,ep,0);
		cp[-1]=0;
		size = (ptrdiff_t)sh_arith((char*)ep+1);
		if(size >= fp->max[n] || (size < 0))
		{
			errormsg(SH_DICT,ERROR_exit(1),e_subscript, nv_name(np));
			UNREACHABLE();
		}
		fp->cur[n] = size;
		cp[-1] = ']';
		sz += fp->cur[n]*fp->incr[n];
	}
skip:
	fp->dim = (size_t)n;
	ap->nelem &= ~ARRAY_MASK;
	ap->nelem |= fp->max[n];
	while(n < (ptrdiff_t)fp->ndim)
		fp->cur[n++] = 0;
	fp->curi = sz;
	return cp-1;
}
#endif /* SHOPT_FIXEDARRAY */

/*
 * process an array subscript for node <np> given the subscript <cp>
 * returns pointer to character after the subscript
 */
char *nv_endsubscript(Namval_t *np, char *cp, nvflag_t mode)
{
	int quoted=0;
	char c;
	ptrdiff_t count=1;
	char *sp = cp+1;
	assert(*cp=='[');
	/* first find matching ']' */
	while(count>0 && (c= *++cp))
	{
		if(c=='\\')
		{
			quoted=1;
			cp++;
		}
		else if(c=='[')
			count++;
		else if(c==']')
			count--;
	}
	*cp = 0;
	if(quoted)
	{
		/* strip escape characters */
		count = stktell(sh.stk);
		sfwrite(sh.stk,sp,1+(size_t)(cp-sp));
		sh_trim(sp=stkptr(sh.stk,count));
	}
	if(mode && np)
	{
		Namarr_t *ap = nv_arrayptr(np);
		/* Block an attempt to alter a readonly array via subscript assignment or by appending the array.
		   However instances of type variables must be allowed. This exception is observed when np->nvflag
		   has NV_BINARY and NV_LJUST set besides NV_RDONLY and NV_ARRAY. */
		if(nv_isattr(np,NV_RDONLY) && nv_isattr(np,NV_ARRAY) && mode&NV_ASSIGN && np->nvflag&(NV_BINARY|NV_LJUST)^(NV_BINARY|NV_LJUST))
		{
			errormsg(SH_DICT,ERROR_exit(1),e_readonly,nv_name(np));
			UNREACHABLE();
		}
#if SHOPT_FIXEDARRAY
		if((mode&NV_FARRAY) && !nv_isarray(np))
		{
			if(array_fixed_init(np,sp,cp+1))
			{
				*cp++ = c;
				return strchr(cp,0);
			}
		}
#endif /* SHOPT_FIXEDARRAY */
		if((mode&NV_ASSIGN) && (cp[1]=='=' || cp[1]=='+'))
			mode |= NV_ADD;
		else if(ap && cp[1]=='.' && (mode&NV_FARRAY))
			mode |= NV_ADD;
#if SHOPT_FIXEDARRAY
		if(ap && ap->fixed)
			cp = array_fixed(np,sp,cp);
		else
#endif /* SHOPT_FIXEDARRAY */
		nv_putsub(np, sp, ((mode&NV_ADD)?ARRAY_ADD:0)|(cp[1]&&(mode&NV_ADD)?ARRAY_FILL:mode&ARRAY_FILL));
	}
	if(quoted)
		stkseek(sh.stk,count);
	*cp++ = c;
	return cp;
}


Namval_t *nv_opensub(Namval_t* np)
{
	Index_array_u ap = { .na = nv_arrayptr(np) };
#if SHOPT_FIXEDARRAY
	struct fixed_array *fp;
#endif /* SHOPT_FIXEDARRAY */
	if(ap.ip)
	{
		if(is_associative(ap.na))
			return (Namval_t*)((*ap.ip->header.fun)(np,NULL,NV_ACURRENT));
#if SHOPT_FIXEDARRAY
		else if(!(fp=ap.ip->header.fixed) && array_isbit(ap.ip->bits,ap.ip->cur,ARRAY_CHILD))
#else
		else if(array_isbit(ap.ip->bits,ap.ip->cur,ARRAY_CHILD))
#endif /* SHOPT_FIXEDARRAY */
		{
			return ap.ip->val[ap.ip->cur];
		}
#if SHOPT_FIXEDARRAY
		else if(fp)
		{
			size_t n = fp->dim;
			if((fp->dim+1) < fp->ndim)
			{
				fp->dim++;
				if(ap.ip->header.nelem&ARRAY_SCAN)
				{
					while(++n < fp->ndim)
						fp->cur[n] = 0;
					fp->level++;
				}
				return np;
			}
		}
#endif /* SHOPT_FIXEDARRAY */
	}
	return NULL;
}

char	*nv_getsub(Namval_t* np)
{
	static char numbuff[NUMSIZE+1];
	Index_array_u ap;
	ptrdiff_t dot, n;
	char *cp = &numbuff[NUMSIZE];
	if(!np || !(ap.na = nv_arrayptr(np)))
		return NULL;
	if(is_associative(ap.na))
		return (char*)((*ap.ip->header.fun)(np,NULL,NV_ANAME));
	if(ap.ip->xp)
	{	/* enum subscript */
		np = nv_namptr(ap.ip->xp,0);
		if(!np->nvalue)
			np->nvalue = sh_malloc(sizeof(uint16_t));
		*((uint16_t*)np->nvalue) = (uint16_t)ap.ip->cur;
		return nv_getval(np);
	}
	if((dot = ap.ip->cur)==0)
		*--cp = '0';
	else while(n=dot)
	{
		dot /= 10;
		*--cp = (char)('0' + (n-10*dot));
	}
	return cp;
}

/*
 * If <np> is an indexed array node, the current subscript index
 * returned, otherwise returns -1
 */
int nv_aindex(Namval_t* np)
{
	Index_array_u ap = { .na = nv_arrayptr(np) };
	if(!ap.na)
		return 0;
	else if(is_associative(ap.na))
		return -1;
#if SHOPT_FIXEDARRAY
	else if(ap.na->fixed)
		return -1;
#endif /* SHOPT_FIXEDARRAY */
	return ap.ip->cur & ARRAY_MASK;
}

ptrdiff_t nv_aimax(Namval_t* np)
{
	Index_array_u ap = { .na = nv_arrayptr(np) };
	ptrdiff_t sub = -1;
#if SHOPT_FIXEDARRAY
	if(!ap.ip || is_associative(&ap.ip->header) || ap.ip->header.fixed)
#else
	if(!ap.ip || is_associative(&ap.ip->header))
#endif /* SHOPT_FIXEDARRAY */
		return -1;
	sub = ap.ip->maxi;
	while(--sub>0 && !ap.ip->val[sub]);
	return sub;
}

/*
 *  This is the default implementation for associative arrays
 */
void *nv_associative(Namval_t *np,const char *sp,nvflag_t mode)
{
	Assoc_array_u ap = { .na = nv_arrayptr(np) };
	nvflag_t type;
	switch(mode)
	{
	    case NV_AINIT:
		ap.asp = sh_calloc(1,sizeof(struct assoc_array));
		ap.asp->header.table = dtopen(&_Nvdisc,Dtoset);
		ap.asp->cur = NULL;
		ap.asp->pos = 0;
		ap.asp->header.hdr.disc = &array_disc;
		nv_disc(np,ap.nfp,NV_FIRST);
		ap.asp->header.hdr.dsize = sizeof(struct assoc_array);
		ap.asp->header.hdr.nofree &= ~1;
		return ap.asp;
	    case NV_ADELETE:
		if(ap.asp->cur)
		{
			if(!ap.asp->header.scope || (Dt_t*)ap.asp->header.scope==ap.asp->header.table || !nv_search(ap.asp->cur->nvname,(Dt_t*)ap.asp->header.scope,0))
				ap.asp->header.nelem--;
			nv_unset(ap.asp->cur,NV_RDONLY);
			nv_delete(ap.asp->cur,ap.asp->header.table,0);
			ap.asp->cur = NULL;
		}
		return ap.asp;
	    case NV_AFREE:
		ap.asp->pos = 0;
		if(ap.asp->header.scope)
		{
			ap.asp->header.table = dtview(ap.asp->header.table,NULL);
			dtclose(ap.asp->header.scope);
			ap.asp->header.scope = NULL;
		}
		else
		{
			if((ap.asp->header.nelem&ARRAY_MASK)==0 && (ap.asp->cur=nv_search("0",ap.asp->header.table,0)))
				nv_associative(np,NULL,NV_ADELETE);
			dtclose(ap.asp->header.table);
			ap.asp->header.table = NULL;
		}
		return ap.asp;
	    case NV_ANEXT:
		if(!ap.asp->pos)
		{
			if((ap.asp->header.nelem&ARRAY_NOSCOPE) && ap.asp->header.scope && dtvnext(ap.asp->header.table))
			{
				ap.asp->header.scope = dtvnext(ap.asp->header.table);
				ap.asp->header.table->view = NULL;
			}
			if(!(ap.asp->pos=ap.asp->cur))
				ap.asp->pos = (Namval_t*)dtfirst(ap.asp->header.table);
		}
		else
			ap.asp->pos = ap.asp->nextpos;
		for(;ap.asp->cur=ap.asp->pos; ap.asp->pos=ap.asp->nextpos)
		{
			ap.asp->nextpos = (Namval_t*)dtnext(ap.asp->header.table,ap.asp->pos);
			if(!nv_isnull(ap.asp->cur))
			{
				if((ap.asp->header.nelem&ARRAY_NOCHILD) && nv_isattr(ap.asp->cur,NV_CHILD))
					continue;
				return ap.asp;
			}
		}
		if((ap.asp->header.nelem&ARRAY_NOSCOPE) && ap.asp->header.scope && !dtvnext(ap.asp->header.table))
		{
			ap.asp->header.table->view = (Dt_t*)ap.asp->header.scope;
			ap.asp->header.scope = ap.asp->header.table;
		}
		return NULL;
	    case NV_ASETSUB:
		ap.asp->cur = (Namval_t*)sp;
		return ap.asp->cur;
	    case NV_ACURRENT:
		if(ap.asp->cur)
			ap.asp->cur->nvmeta = np;
		return ap.asp->cur;
	    case NV_ANAME:
		if(ap.asp->cur)
		{
			if(!sh.instance && nv_isnull(ap.asp->cur))
				return NULL;
			return ap.asp->cur->nvname;
		}
		return NULL;
	    default:
		if(sp)
		{
			Namval_t *mp = NULL;
			ap.asp->cur = 0;
			if(sp==(char*)np)
				return NULL;
			type = nv_isattr(np,NV_PUBLIC&~(NV_ARRAY|NV_CHILD|NV_MINIMAL));
			if(mode)
				mode = NV_ADD|NV_NOSCOPE;
			else if(ap.asp->header.nelem&ARRAY_NOSCOPE)
				mode = NV_NOSCOPE;
			if(*sp==0 && sh_isoption(SH_XTRACE) && (mode&NV_ADD))
				errormsg(SH_DICT,ERROR_warn(0),"adding empty subscript");
			if(sh.subshell && (mp=nv_search(sp,ap.asp->header.table,0)) && nv_isnull(mp))
				ap.asp->cur = mp;
			if((mp || (mp=nv_search(sp,ap.asp->header.table,mode))) && nv_isnull(mp) && (mode&NV_ADD))
			{
				nv_onattr(mp,type);
				mp->nvmeta = np;
				if((mode&NV_ADD) && nv_type(np))
					nv_arraychild(np,mp,0);
				if(sh.subshell)
					sh_assignok(np,1);
				/*
				 * For enum types (NV_UINT16 with discipline ENUM_disc), nelem should not
				 * increase or 'unset' will fail to completely unset such an array.
				 */
				if((!ap.asp->header.scope || !nv_search(sp,dtvnext(ap.asp->header.table),0))
				&& !(type==NV_UINT16 && nv_hasdisc(np, &ENUM_disc)))
					ap.asp->header.nelem++;
				if(nv_isnull(mp))
				{
					if(ap.asp->header.nelem&ARRAY_TREE)
						nv_setvtree(mp);
					mp->nvalue = Empty;
				}
			}
			else if(ap.asp->header.nelem&ARRAY_SCAN)
			{
				Namval_t fake;
				fake.nvname = (char*)sp;
				ap.asp->pos = mp = (Namval_t*)dtprev(ap.asp->header.table,&fake);
				ap.asp->nextpos = (Namval_t*)dtnext(ap.asp->header.table,mp);
			}
			else if(!mp && *sp && mode==0)
				mp = nv_search(sp,ap.asp->header.table,NV_ADD|NV_NOSCOPE);
			np = mp;
			if(ap.asp->pos && ap.asp->pos==np)
				ap.asp->header.nelem |= ARRAY_SCAN;
			else if(!(ap.asp->header.nelem&ARRAY_SCAN))
				ap.asp->pos = 0;
			ap.asp->cur = np;
		}
		if(ap.asp->cur)
			return &ap.asp->cur->nvalue;
		else
			return &ap.asp->cur;
	}
}

/*
 * Assign values to an array
 */
void nv_setvec(Namval_t *np,int append,int argc,char *argv[])
{
	ptrdiff_t arg0 = 0;
	struct index_array *aq;
	Index_array_u ap = { .ip = NULL };
	if(nv_isarray(np))
	{
		ap.na = nv_arrayptr(np);
		if(ap.ip && is_associative(ap.na))
		{
			errormsg(SH_DICT,ERROR_exit(1),"cannot append indexed array to associative array %s",nv_name(np));
			UNREACHABLE();
		}
	}
	if(append)
	{
		if(ap.ip)
		{
			if(!(aq = (struct index_array*)ap.ip->header.scope))
				aq = ap.ip;
			arg0 = ap.ip->maxi;
			while(--arg0>0 && !ap.ip->val[arg0] && !aq->val[arg0]);
			arg0++;
		}
		else
		{
			nv_offattr(np,NV_ARRAY);
			if(!nv_isnull(np) && np->nvalue!=Empty)
				arg0=1;
		}
	}
	while(--argc >= 0)
	{
		nv_putsub(np,NULL,(long)argc+arg0|ARRAY_FILL|ARRAY_ADD);
		nv_putval(np,argv[argc],0);
	}
}
