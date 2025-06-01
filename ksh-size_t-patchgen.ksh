#!/bin/ksh -e
# SPDX identifier: EPL 2.0
# Generate a patch series for the monstrous size_t fixes.
# This ksh93 script is only meant for internal development use.

if ((.sh.version < 20250520)); then
	echo 'ksh93 too old; not bothering...'
	exit 1
fi

git reset --hard; git clean -fdx
git checkout dev
git branch -D 64bit-fixes-series || true
git branch 64bit-fixes-series
git checkout 64bit-fixes-series
rm -f ksh-size_t-patchgen.ksh
alias fetch='git checkout thickfold-size_t --'
alias unfetch='git checkout HEAD --'
if [[ $1 == --sanity ]]; then
	alias sanity='bin/package make CC=tcc -j12 -c && bin/shtests -u'
else
	alias sanity=true
fi
upc() {
	# Obtain the script from the from ksh wiki
	test -f ../update-copyright.ksh && ksh ../update-copyright.ksh
	git checkout HEAD -- COPYRIGHT
	git add bin src
}

export GIT_AUTHOR_EMAIL='johnothanking@protonmail.com'
export GIT_AUTHOR_NAME='Johnothan King'

fetch src/cmd/ksh93/include/test.h
fetch src/cmd/ksh93/include/defs.h
fetch src/cmd/ksh93/include/shell.h
fetch src/cmd/ksh93/sh/macro.c
fetch src/cmd/ksh93/sh/init.c
fetch src/cmd/ksh93/bltins/test.c
fetch src/lib/libast/man/stk.3
fetch src/lib/libast/include/stk.h
fetch src/lib/libast/misc/stk.c
fetch src/cmd/ksh93/shell.3
sanity
git commit -m $'size_t/ptrdiff_t transition part 1: test(1), .sh.match, macro expansion, init and stk(3)

This is the first of a thirteen(!!) part patch series that enables
ksh93 to operate withing a 64-bit address space (currently dubbed
thickfold). These changes were accomplished by fixing most (but not
all) of the warnings that materialize during compilation when the
following clang compiler flags are passed:
\'-Wsign-compare -Wshorten-64-to-32 -Wsign-conversion -Wimplicit-int-conversion\'

Originally, this patch was going to take the suggested approach of
replacing int with ssize_t. While that does work in practice, it\'s
a bad idea because POSIX does not guarantee ssize_t will accept
any negative value besides -1, despite its signed nature:
    > The type ssize_t shall be capable of storing values at least
    > in the range [-1, {SSIZE_MAX}].
As such, for correctness and portability this commit will prefer
usage of the C89 ptrdiff_t, which is practically guaranteed to
accept the full range of negative numbers an int can accept. It is
possible for ptrdiff_t to be larger than size_t (i.e. being 64-bit
whilst size_t is 32-bit), but that isn\'t really a concern seeing as
this patch series aims to improve support for platforms with
64-bit size_t (ksh currently suffers from archaic 32-bit int usage).

In any case expanding int to ptrdiff_t should be of relatively low
risk for the reasons above, but size_t was used in certain locations
when it was more appropriate (although its unsignedness necessitates
greater caution because of rollover concerns). Additionally, some
sections already use the ssize_t type (e.g. strgrpmatch()), so for
those cases ssize_t usage has been maintained.

Also of note, in some areas ssize_t results from e.g. read(2)
may end up being used with ptrdiff_t variables. This is not
pedantically correct, but it\'s certainly better than the prior
int hell status quo.

Conspicuous changes with noteworthyness:
- Fixed many, many compiler warnings by adding a considerable
  number of casts and changing the types of more than quite
  a few variables. This is a consequence of using compiler
  warnings as an aid for implementing 64-bit memory allocation;
  while the warnings were useful, there were so many warnings
  fixed in the process that it increased the patch sizes greatly.
- Transitioned the strgrpmatch() and strngrpmatch calls to use
  a ssize_t* pointer rather than an int* pointer.
- For the sh_options macros/function, enforce uint64_t as the main
  argument type to fix compiler warnings.
- The libast stk sublibrary already uses the ssize_t/size_t types,
  which made it rather easy to integrate proper ptrdiff_t usage.
  In fact, stktell() has *always* returned a \'ptrdiff_t\' result.
  The documentation in stk(3) has been incorrectly claiming
  since < 1995 it returns \'int\', which is wrong. That error
  has been rectified alongside the multitude of updates to stk.


Side note: To re-emphasize, this patch is one part of a whole
(although it can be used on it\'s own). The full suite of
changes can be found on the thickfold-size_t branch.
This first part has been submitted severed from the other
changes to make code review less laborious (I hope).

Progresses https://github.com/ksh93/ksh/issues/592'


fetch src/lib/libast/sfio
fetch src/lib/libast/stdio
fetch src/lib/libast/path/pathgetlink.c
fetch src/lib/libast/man/sfio.3
fetch src/lib/libast/man/strmatch.3
fetch src/lib/libast/include/sfio.h
fetch src/lib/libast/include/ast.h
fetch src/lib/libast/include/modex.h
fetch src/lib/libast/include/sfio_t.h
fetch src/lib/libast/include/sfio_s.h
fetch src/lib/libast/man/modecanon.3
fetch src/lib/libast/string/fmtmode.c
fetch src/lib/libast/string/modelib.h
fetch src/lib/libast/string/modei.c
fetch src/lib/libast/string/modex.c
fetch src/lib/libast/string/fmtmode.c
fetch src/lib/libast/string/strmode.c
fetch src/lib/libast/string/fmtperm.c
fetch src/lib/libast/string/modedata.c
fetch src/lib/libast/include/regex.h
fetch src/lib/libast/include/hash.h
fetch src/lib/libast/string/strperm.c
fetch src/lib/libast/man/strperm.3
fetch src/lib/libast/hash
fetch src/lib/libast/string/strmatch.c
fetch src/lib/libast/man/fmt.3
fetch src/lib/libast/string/fmtelapsed.c
fetch src/lib/libast/string/fmtmode.c
fetch src/cmd/ksh93/bltins/print.c
fetch src/lib/libast/features/stdio
fetch src/lib/libast/man/hash.3
fetch src/lib/libast/man/path.3
sanity
git commit -m $'size_t/ptrdiff_t transition part 2: SFIO, hash lib and print(1)

This is the second of the thickfold patch series, which enables ksh93
to operate within a 64-bit address space.

The parts of ksh93 affected by this commit are:
- SFIO and the associated stdio wrapper. The sfvprintf and sfvscanf
  functions are notable for bearing the brunt of the substantial code
  changes.
  - Nearly all of the code is de novo, though towards the latter end
    of development I used graphviz commits for cross-reference:
    - https://gitlab.com/graphviz/graphviz/-/commit/6451a669    
    - https://gitlab.com/graphviz/graphviz/-/commit/153a8f87
    - https://gitlab.com/graphviz/graphviz/-/commit/cb9b35d1
  - sfseek(): Removed a wasteful double assignment for f->iosz.
    This error was introduced in 2003-06-21 ksh93o+, and likely
    would have gone unnoticed if not for the thickfold project.
    (In practice this line virtually always optimized out by the
    compiler, so it was dead code.)
- 64-bit modernization for the AST hash library.
- Transitioned pathgetlink() to return ssize_t.
- Updated the code in print.c for compatibility with the SFIO 64-bit
  modernization.
- Transitioned the strgrpmatch() family to use regflags_t to appease
  some -Wsign-conversion warnings (the type is now located in ast.h).
  - The now unused STR_INT flag backported from ksh93v- has been
    removed to avoid bitrot.
- Removed the unused nonstandard ssizeof() macro from ast.h.
- Made the snprintf and vsnprintf wrappers standards compliant
  with the C standard.

Progresses https://github.com/ksh93/ksh/issues/592'



fetch src/lib/libast/aso
fetch src/lib/libast/cdt
fetch src/lib/libast/features
unfetch src/lib/libast/features/tty
fetch src/lib/libast/include/aso.h
fetch src/lib/libast/man/aso.3
fetch src/lib/libast/include/cdt.h
sanity
git commit -m 'size_t/ptrdiff_t transition part 3: aso, CDT, libast feature tests

This is the third of the thickfold patch series, which enables ksh93
to operate within a 64-bit address space.

The parts of ksh93 affected by this commit are:
- The libast aso library.
- The libast CDT library.
- The libast feature tests.

Progresses https://github.com/ksh93/ksh/issues/592'


fetch src/lib/libast/regex/                                                                                                                                                                                                                                             
fetch src/lib/libast/string/
fetch src/lib/libast/include/regex.h
fetch src/lib/libast/include/swap.h
fetch src/lib/libast/man/swap.3
fetch src/lib/libast/man/regex.3
sanity
git commit -m 'size_t/ptrdiff_t transition part 4: libast regex and string sublibraries

This is the fourth of the thickfold patch series, which enables ksh93
to operate within a 64-bit address space.

The parts of ksh93 affected by this commit are:
- The libast string library.
- The libast regex engine.

Progresses https://github.com/ksh93/ksh/issues/592'



fetch src/lib/libast/comp/
fetch src/lib/libast/misc/
fetch src/lib/libast/port/
fetch src/lib/libast/include/cmdarg.h
fetch src/lib/libast/include/glob.h
fetch src/lib/libast/features/api
fetch src/lib/libast/std/assert.h
fetch src/lib/libast/include/debug.h
sanity
git commit -m $'size_t/ptrdiff_t transition part 5: the libast zakkaya

This is the fifth of the thickfold patch series, which enables ksh93
to operate within a 64-bit address space.

The parts of ksh93 affected by this commit are:
- The libast compatibility functions.
- The libast miscellaneous functions (e.g. optget and friends).
- The libast portability functions.

Remarks:
- The second argument of getfsstat can either be a long (FreeBSD)
  or a size_t (OpenBSD). This patch opts to cast the second argument
  as a size_t, which will produce a warning on FreeBSD and no warning
  on OpenBSD.
- _ast_assertfail and debug_fatal() end with abort(), so they were
  marked with the noreturn attribute.
- error_break() triggered a warning because it\'s given extern
  despite being uses as though it were static, so it was
  marked static to reflect actual usage.
  - typefix() was not extern, but was missing its static designation,
    so that has also been rectified.
- Removed cmdopen_20110505() to avoid bitrot.

Progresses https://github.com/ksh93/ksh/issues/592'

fetch src/lib/libast
sanity
git commit -m $'size_t/ptrdiff_t transition part 6: remainder of additaments to libast

This is the sixth of the thickfold patch series, which enables ksh93
to operate within a 64-bit address space.

The parts of ksh93 affected by this commit are:
- The the reminder of SFIO located in the disc folder.
  - Removed sfkeyprintf_20000308() to prevent bitrot.
- The libast tm and tmx sublibraries.
- The rest of the libast path sublibrary.
- The rest of the libast headers.
- The vmalloc wrapper\'s header file.
- The AST dirlib (where ssize_t was more appropriate that ptrdiff_t).

Progresses https://github.com/ksh93/ksh/issues/592'

fetch src/lib/libast/features/tty
fetch src/lib/libcmd
sanity
git commit -m $'size_t/ptrdiff_t transition part 7: libcmd builtins

This is the seventh of the thickfold patch series, which enables ksh93
to operate within a 64-bit address space.

The parts of ksh93 affected by this commit are:
- The entirety of the libcmd builtins.

Remarks:
- Like the previous problematic getfsstat, sethostname also
  has a platform-dependent second argument that can be
  either signed or unsigned. This commit leaves the usage
  of that function (which passes a size_t value) as is.
- Moved the include directives in chmod and date to
  silence overlength string warnings.
- Removed unused macros from the fds code.
- Marked unused function parameters in grep to silence
  additional warnings.
- Use POSIX tcflag_t, speed_t and cc_t to silence
  warnings in stty.

Progresses https://github.com/ksh93/ksh/issues/592'

fetch src/lib/libdll
fetch src/lib/libsum
fetch src/cmd/builtin
fetch src/cmd/INIT
sanity
git commit -m $'size_t/ptrdiff_t transition part 8: ancillary AST suite components

This is the eight of the thickfold patch series, which enables ksh93
to operate within a 64-bit address space.

The parts of ksh93 affected by this commit are:
- The entirety of the libdll library.
  - An unused dllopen() wrapper hidden behind \'\#if 0\' has been
    removed to prevent bitrot.
- The entirety of the libsum library.
  - match(): Added an UNREACHABLE() to fix a -Wunreachable-code-return
    warning.
- The pty test utility.
- The mamake build program.

Progresses https://github.com/ksh93/ksh/issues/592'

fetch src/cmd/ksh93/edit
fetch src/cmd/ksh93/include/edit.h
fetch src/cmd/ksh93/include/history.h
sanity
git commit -m $'size_t/ptrdiff_t transition part 9: ksh93 command line editors

This is the ninth of the thickfold patch series, which enables ksh93
to operate within a 64-bit address space.

The parts of ksh93 affected by this commit are:
- The interactive emacs and vi editor modes.
  Neither possess an exigent need to operate on exceedingly large
  buffers (I\'d be surprised if they ever needed to operate in
  gigabytes to begin with), so I opted to liberally use int
  casts to merely fix compiler warnings.
  - Moved a FALLTHROUGH comment to fix a fallthrough warning.

Progresses https://github.com/ksh93/ksh/issues/592'

fetch src/cmd/ksh93/bltins
fetch src/cmd/ksh93/include/builtins.h
sanity
git commit -m $'size_t/ptrdiff_t transition part 10: ksh93 preeminent builtin commands

This is the tenth of the thickfold patch series, which enables ksh93
to operate within a 64-bit address space.

The parts of ksh93 affected by this commit are:
- The various primary ksh93 builtins located in the bltins folder.
  - print_namval() returns a value only used for boolean tests,
    so it doesn\'t need to return nv_size + 1 and thus a warning
    can be quashed.

Progresses https://github.com/ksh93/ksh/issues/592'

fetch src/cmd/ksh93/sh/n*
fetch src/cmd/ksh93/include/n*
fetch src/cmd/ksh93/sh/array.c
fetch src/cmd/ksh93/sh/string.c
fetch src/cmd/ksh93/sh/waitevent.c
fetch src/cmd/ksh93/nval.3
sanity
git commit -m "size_t/ptrdiff_t transition part 11: ksh93 variables

This is the eleventh of the thickfold patch series, which enables ksh93
to operate within a 64-bit address space.

The parts of ksh93 affected by this commit are:
- The nval system, string.c, array handling and waitevent.c.
  This is probably one of the most consequential changes, the
  effect being that ksh is now capable of storing variables
  larger than UINT_MAX.

Conspicuous changes of note:
- With the expansion of np->nvsize to a size_t (which can
  be either 64-bit or 32-bit), the Sfdouble_t* padding
  was a necessary addition for alignment purposes, so as to
  silence UBSan errors and prevent untimely memory faults.
- An -Wunreachable-code-return warning at the end of nv_create()
  has been fixed.
- nv_setsize() as exposed in the public nval API now requires
  an argument of exactly (size_t)-1 to function like the
  nv_size() macro.
- nv_create(): ported over an old fix from my expand-nvflags branch
  to allow usage of strlen without downcasting the result to an int.
  This fix adds a uint64_t nvflags variable for storing flags, which
  introduces some inconsequential warnings. Fixing/avoiding those is
  out of the scope of this project; that\'s better suited for a
  future revision of expand-nvflags.


Progresses https://github.com/ksh93/ksh/issues/592"


fetch src/cmd/ksh93/sh/t*
fetch src/cmd/ksh93/sh/parse.c
fetch src/cmd/ksh93/sh/lex.c
fetch src/cmd/ksh93/sh/fcin.c
fetch src/cmd/ksh93/sh/subshell.c
fetch src/cmd/ksh93/sh/shcomp.c
fetch src/cmd/ksh93/include/fcin.h
fetch src/cmd/ksh93/include/shnodes.h
fetch src/cmd/ksh93/include/shlex.h
sanity
git commit -m "size_t/ptrdiff_t transition part 12: ksh93 lexing, parsing and subshells

This is the twelfth of the thickfold patch series, which enables ksh93
to operate within a 64-bit address space.

The parts of ksh93 affected by this commit are:
- The lexing and parsing components in lex.c, parse.c, fcin.c
  and trestore.c.
  - Removed the set but not used fcleft variable.
- Minor fixes for the virtual subshell mechanism.
- The code underlying shcomp(1), aka sh_tdump().
- A minor fix to a cast in sh_timeradd().


Progresses https://github.com/ksh93/ksh/issues/592"


fetch src
sanity
sed -i '5i '${ printf '%(%Y-%0m-%0d)T\n' now ;}':\n\n- Ksh is now capable of allocating memory within a 64-bit address space.\n' NEWS
git add NEWS
upc
git commit -m "size_t/ptrdiff_t transition part 13: the rest of ksh93

This is the thirteenth of the thickfold patch series, which enables ksh93
to operate within a 64-bit address space.

This covers the rest of ksh93:
- All of the other headers.
- args.c
- arith.c
- deparse.c
- expand.c
- fault.c
- io.c
  - In this file ssize_t is preferable because many of the
    underlying SFIO and POSIX function return values of
    that type.
  - sh_sfeval(): removed the set but not used ep->slen variable.
- jobs.c
  - Get rid of if/else PID botch; a single casted strtoll is fine.
- main.c
- path.c
  - Use the order 'noreturn void' for exscript() to fix a
    compiler warning.
  - path_spawn(): Added an UNREACHABLE() to fix an instance of
    -Wunreachable-code-return.
- streval.c
- xec.c

Conspicuous change of note:
- Removed an (unsigned) cast for sizeof because it's already
  unsigned.

Fixes https://github.com/ksh93/ksh/issues/592"

git format-patch -k dev..64bit-fixes-series
