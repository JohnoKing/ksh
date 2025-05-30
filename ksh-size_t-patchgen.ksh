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
if [[ $1 == --sanity ]]; then
	alias sanity='bin/package make CC=tcc -j12 -c; bin/shtests -u'
else
	alias sanity=true
fi

export GIT_AUTHOR_EMAIL='johnothanking@protonmail.com'
export GIT_AUTHOR_NAME='Johnothan King'

### TODO: Integrate test.c, macro.c, init.c and defs.h into this commit ###

fetch src/lib/libast/sfio
fetch src/lib/libast/stdio
fetch src/lib/libast/path/pathgetlink.c
fetch src/lib/libast/man/sfio.3
fetch src/lib/libast/man/strmatch.3
fetch src/lib/libast/include/sfio.h
fetch src/lib/libast/include/ast.h
fetch src/lib/libast/include/modex.h
fetch src/lib/libast/man/modecanon.3
fetch src/lib/libast/string/fmtmode.c
fetch src/lib/libast/string/modelib.h
fetch src/lib/libast/string/modei.c
fetch src/lib/libast/string/modex.c
fetch src/lib/libast/string/fmtmode.c
fetch src/lib/libast/string/fmtperm.c
fetch src/lib/libast/string/modedata.c
fetch src/lib/libast/include/regex.h
fetch src/lib/libast/include/hash.h
fetch src/cmd/ksh93/include/test.h
fetch src/lib/libast/string/strperm.c
fetch src/lib/libast/hash
fetch src/cmd/ksh93/bltins/print.c
fetch src/lib/libast/string/strmatch.c
fetch src/lib/libast/man/fmt.3
fetch src/cmd/ksh93/include/defs.h
fetch src/cmd/ksh93/include/shell.h
fetch src/cmd/ksh93/sh/macro.c
fetch src/cmd/ksh93/sh/init.c
fetch src/cmd/ksh93/bltins/test.c
fetch src/lib/libast/string/fmtelapsed.c
fetch src/lib/libast/string/fmtmode.c
sanity
git commit -m $'ssize_t transition part 1: SFIO, hash lib and print(1)

This is the first of a patch series that enables ksh93 to operate
withing a 64-bit address space. These changes were accomplished by
fixing most of the -Wsign-compare and -Wshorten-64-to-32 warnings
exhibited during compilation with clang. ssize_t was used most
often to avoid rollover issues (int and ssize_t are both signed),
but in some cases size_t was used instead to fix the aforementioned
-Wsign-compare warnings. (There are still many -Wsign-conversion
warnings, and while the majority of those have been fixed, many were
left as is; some are dependent on the OS\'s system headers and
cannot feasibly be fixed.)

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
    would have gone unnoticed if not for this ssize_t project.
    (In practice this line virtually always optimized out by the
    compiler, so it is dead code.)
- ssize_t modernization for the AST hash library.
  - The formerly unused ssizeof() macro now casts to ssize_t rather
    than int (the previous behavior is contra least surprise).
- Transitioned pathgetlink() to ssize_t.
- Updated the code in print.c for compatibility with the SFIO ssize_t
  modernization.
- Transitioned the strgrpmatch() family to use regflags_t to fix
  various -Wsign-conversion warnings.
  - In accompaniment with those changes, applied ssize_t updates to
    macro.c, init.c and test(1).
  - The STR_INT flag backported from ksh93v- was left unused due
    to these changes, so it has been removed.
- Made the snprintf and vsnprintf wrappers standards compliant.

Progresses https://github.com/ksh93/ksh/issues/592'



fetch src/lib/libast/aso
fetch src/lib/libast/cdt
fetch src/lib/libast/features
fetch src/lib/libast/include/aso.h
sanity
git commit -m 'ssize_t transition part 2: aso, CDT, libast feature tests

The is the second in the ssize_t transition patch series.
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
sanity
git commit -m 'ssize_t transition part 3: libast regex and string sublibraries

The is the third in the ssize_t transition patch series.
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
sanity
git commit -m 'ssize_t transition part 4: the libast zakkaya

The is the fourth in the ssize_t transition patch series.
The parts of ksh93 affected by this commit are:
- The libast compatibility functions.
- The libast miscellaneous functions (e.g. optget and friends).
- The libast portability functions.

Progresses https://github.com/ksh93/ksh/issues/592'

fetch src/lib/libast
sanity
git commit -m 'ssize_t transition part 5: remainder of additaments to libast

The is the fifth in the ssize_t transition patch series.
The parts of ksh93 affected by this commit are:
- The the reminder of SFIO located in the disc folder.
- The rest of the libast man pages.
- The libast tm and tmx sublibraries.
- The rest of the libast path sublibrary.
- The rest of the libast headers.

Progresses https://github.com/ksh93/ksh/issues/592'

fetch src/lib/libcmd
sanity
git commit -m 'ssize_t transition part 6: libcmd builtins

The is the sixth in the ssize_t transition patch series.
The parts of ksh93 affected by this commit are:
- The entirety of the libcmd builtins.

Progresses https://github.com/ksh93/ksh/issues/592'

fetch src/lib/libdll
fetch src/lib/libsum
fetch src/cmd/builtin
fetch src/cmd/INIT
sanity
git commit -m $'ssize_t transition part 7: ancillary AST suite components of ksh93

The is the seventh in the ssize_t transition patch series.
The parts of ksh93 affected by this commit are:
- The entirety of the libdll library.
  - Unused code hidden behind \'\#if 0\' has been removed
    to prevent bitrot.
- The entirety of the libsum library.
- The pty test utility.
- The mamake build program.

Progresses https://github.com/ksh93/ksh/issues/592'

fetch src/cmd/ksh93/edit
fetch src/cmd/ksh93/include/edit.h
fetch src/cmd/ksh93/include/history.h
sanity
git commit -m $'ssize_t transition part 8: ksh93 command line editors

The is the eighth in the ssize_t transition patch series.
The parts of ksh93 affected by this commit are:
- The interactive emacs and vi editor modes.
  Neither possess an exigent need to operate on exceedingly large
  buffers (I\'d be surprised if they ever needed to operate in
  gigabytes to begin with), so for the most part int casts
  were utilized merely to fix compiler warnings.

Progresses https://github.com/ksh93/ksh/issues/592'

fetch src/cmd/ksh93/bltins
fetch src/cmd/ksh93/include/builtins.h
sanity
git commit -m 'ssize_t transition part 9: ksh93 preeminent builtin commands

The is the ninth in the ssize_t transition patch series.
The parts of ksh93 affected by this commit are:
- The various primary ksh93 builtins located in the bltins folder.

Progresses https://github.com/ksh93/ksh/issues/592'

fetch src/cmd/ksh93/sh/n*
fetch src/cmd/ksh93/include/n*
fetch src/cmd/ksh93/sh/array.c
fetch src/cmd/ksh93/sh/string.c
fetch src/cmd/ksh93/sh/waitevent.c
fetch src/cmd/ksh93/nval.3
sanity
git commit -m "ssize_t transition part 10: ksh93 variables

The is the tenth in the ssize_t transition patch series.
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

Progresses https://github.com/ksh93/ksh/issues/592"


fetch src/cmd/ksh93/sh/t*
fetch src/cmd/ksh93/sh/parse.c
fetch src/cmd/ksh93/sh/lex.c
fetch src/cmd/ksh93/sh/fcin.c
fetch src/cmd/ksh93/include/fcin.h
fetch src/cmd/ksh93/include/shnodes.h
sanity
git commit -m "ssize_t transition part 11: ksh93 lexing and parsing

The is the eleventh in the ssize_t transition patch series.
The parts of ksh93 affected by this commit are:
- All C files starting with the letter 't' in the sh folder.
- The lexing and parsing components in lex.c, parse.c, and fcin.c.

Progresses https://github.com/ksh93/ksh/issues/592"


fetch src
sanity
sed -i '5i '${ printf '%(%Y-%0m-%0d)T\n' now ;}':\n\n- Ksh is now capable of allocating memory within a 64-bit address space.\n' NEWS
git add NEWS
git commit -m "ssize_t transition part 12: the rest of ksh93

The is the twelfth in the ssize_t transition patch series.
This covers the rest of ksh93:
- All of the other headers.
- args.c
- arith.c
- expand.c
- fault.c
- io.c
- jobs.c
- main.c
- path.c
- streval.c
- xec.c

Conspicuous changes of note:
- Incidentally fixed a compiler error for SHOPT_KIA by adding the
  necessary include directive for shlex.h to defs.c.
- Removed an (unsigned) cast for sizeof because it's already
  unsigned.

Fixes https://github.com/ksh93/ksh/issues/592"

git format-patch -k dev..64bit-fixes-series
