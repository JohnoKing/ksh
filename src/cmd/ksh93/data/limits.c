/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2025 Contributors to ksh 93u+m           *
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
*                                                                      *
***********************************************************************/

#include	"shopt.h"
#include	<ast.h>
#include	"ulimit.h"

/*
 * This is the list of resource limits controlled by ulimit
 * This command requires getrlimit(), vlimit(), or ulimit()
 */

#ifndef _no_ulimit

const char	e_unlimited[] = "unlimited";
const char*	e_units[] = { 0, "block", "byte", "Kibyte", "second", "microsecond" };

const int	shtab_units[] = { 1, 512, 1, 1024, 1, 1 };

const Limit_t	shtab_limits[] =
{
"as",       "address space limit",           0,              RLIMIT_AS,         'M', LIM_KBYTE,
"core",     "core file size",                0,              RLIMIT_CORE,       'c', LIM_BLOCK,
"cpu",      "cpu time",                      0,              RLIMIT_CPU,        't', LIM_SECOND,
"data",     "data size",                     0,              RLIMIT_DATA,       'd', LIM_KBYTE,
"fsize",    "file size",                     0,              RLIMIT_FSIZE,      'f', LIM_BLOCK,
"kqueues",  "number of kqueues",             0,              RLIMIT_KQUEUES,    'k', LIM_COUNT,
"locks",    "number of file locks",          0,              RLIMIT_LOCKS,      'x', LIM_COUNT,
"memlock",  "locked address space",          0,              RLIMIT_MEMLOCK,    'l', LIM_KBYTE,
"msgqueue", "message queue size",            0,              RLIMIT_MSGQUEUE,   'q', LIM_KBYTE,
"nice",     "scheduling priority",           0,              RLIMIT_NICE,       'e', LIM_COUNT,
"nofile",   "number of open files",          "OPEN_MAX",     RLIMIT_NOFILE,     'n', LIM_COUNT,
"novmon",   "number of open vnode monitors", 0,              RLIMIT_NOVMON,     'V', LIM_COUNT,
"nproc",    "number of processes",           "CHILD_MAX",    RLIMIT_NPROC,      'u', LIM_COUNT,
"npts",     "number of pseudo-terminals",    0,              RLIMIT_NPTS,       'P', LIM_COUNT,
"pipe",     "pipe buffer size",              "PIPE_BUF",     RLIMIT_PIPE,       'p', LIM_BYTE,
"rss",      "max memory size",               0,              RLIMIT_RSS,        'm', LIM_KBYTE,
"rtprio",   "max real-time priority",        0,              RLIMIT_RTPRIO,     'r', LIM_COUNT,
"rttime",   "max time before blocking",      0,              RLIMIT_RTTIME,     'R', LIM_MICROSECOND,
"sbsize",   "socket buffer size",            "PIPE_BUF",     RLIMIT_SBSIZE,     'b', LIM_BYTE,
"sigpend",  "signal queue size",             "SIGQUEUE_MAX", RLIMIT_SIGPENDING, 'i', LIM_COUNT,
"stack",    "stack size",                    0,              RLIMIT_STACK,      's', LIM_KBYTE,
"swap",     "swap size",                     0,              RLIMIT_SWAP,       'w', LIM_KBYTE,
"threads",  "number of threads",             "THREADS_MAX",  RLIMIT_PTHREAD,    'T', LIM_COUNT,
"vmem",     "process size",                  0,              RLIMIT_VMEM,       'v', LIM_KBYTE,
{ 0 }
};

#endif
