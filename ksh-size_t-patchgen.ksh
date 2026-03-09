#!/bin/ksh -e
# SPDX identifier: EPL 2.0
# Generate a patch series for the monstrous size_t fixes.
# This ksh93 script is only meant for internal development use.

if ((.sh.version < 20250520)); then
	echo 'ksh93 too old; not bothering...'
	exit 1
fi

integer -a w=(-0 -1 -2 -3 -4 -5 -6 -7 -8 -9 -10 -11 -12 -13)
integer cores=${ bin/package host cpu ;}
bld() {
	rm -rf ./arch
	bin/package make CC=clang CCFLAGS='-O0 -Wsign-compare -Wshorten-64-to-32 -Wsign-conversion -Wimplicit-int-conversion' -j${cores} >/tmp/buildlog
	w[++iter]=${ grep 'warning:' /tmp/buildlog | wc -l ;}
	if (( iter < 8 )); then
		w[iter]+=32  # Add the initial mamake.c warnings not present in /tmp/buildlog
	fi
}

git reset --hard; git clean -fdx
git checkout thickfold-size_t
integer iter=12
bld
iter=-1
git checkout a215b8be7958974902299233231611165b132114
typeset dev_commit=${ git log --pretty=format:'%h' -n 1 --abbrev-commit ;}
bld

# Get warnings for part one
git checkout 33e66eac603f8203bc3239b251417a02a3762946
bld

git branch -D 64bit-fixes-series 2>/dev/null || true
git checkout -b 64bit-fixes-series
rm -f ksh-size_t-patchgen.ksh
alias fetch='git checkout thickfold-size_t --'
alias unfetch='git checkout HEAD --'
if [[ $1 == --sanity ]]; then
	typeset sanity=true
fi

upc() {
	# Obtain the script from the from ksh wiki
	test -f ../update-copyright.ksh && ksh ../update-copyright.ksh
	if [[ $1 != --all ]]; then
		git checkout HEAD -- COPYRIGHT
	else
		git add COPYRIGHT
	fi
	git add bin src
	bld
	if [[ $sanity == true ]]; then
		bin/shtests || {
			print 'Tests failed; waiting...'
			read
		}
	fi
}

export GIT_AUTHOR_EMAIL='johnothanking@protonmail.com'
export GIT_AUTHOR_NAME='Johnothan King'

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
fetch src/lib/libast/string/fmtscale.c
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
upc
git commit -m $'size_t/ptrdiff_t transition part 2: SFIO, hash lib, print(1), fmt*(), strmatch()

This is the second of the thickfold patch series, which enables ksh93
to operate within a 64-bit address space. This part of the patch series
includes everything affected by changes to <ast.h>.

The parts of ksh93 affected by this commit are:
- SFIO and the associated stdio wrapper. The sfvprintf and sfvscanf
  functions are notable for bearing the brunt of the substantial code
  changes.
  - Nearly all of the code is de novo, though towards the latter end
    of development I used graphviz commits for cross-reference:
    - https://gitlab.com/graphviz/graphviz/-/commit/6451a669
    - https://gitlab.com/graphviz/graphviz/-/commit/153a8f87
    - Improved multibyte handling in sfvscanf via use of
      unsigned char*; ported from graphviz:
      https://gitlab.com/graphviz/graphviz/-/commit/cb9b35d1
      (I\'m aware this function is unused, but after some manual
      testing it works well AFAICT. A bit odd that ksh93 never
      uses it once.)
  - sfseek(): Removed a wasteful double assignment for f->iosz.
    This error was introduced in 2003-06-21 ksh93o+, and likely
    would have gone unnoticed if not for the thickfold project.
    (In practice this line was virtually always optimized out by the
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

Change in the number of warnings on Linux when compiling with clang using
-Wsign-compare -Wshorten-64-to-32 -Wsign-conversion -Wimplicit-int-conversion:
'"${ printf "%'d => %'d => %'d" ${w[1]} ${w[2]} ${w[13]} ;}"' (progression from part 1 => part 2 => part 13)

Progresses https://github.com/ksh93/ksh/issues/592'



fetch src/lib/libast/aso
fetch src/lib/libast/cdt
fetch src/lib/libast/features
unfetch src/lib/libast/features/tty
fetch src/lib/libast/include/aso.h
fetch src/lib/libast/man/aso.3
fetch src/lib/libast/include/cdt.h
upc
git commit -m 'size_t/ptrdiff_t transition part 3: aso, CDT, libast feature tests

This is the third of the thickfold patch series, which enables ksh93
to operate within a 64-bit address space.

The parts of ksh93 affected by this commit are:
- The libast aso library.
- The libast CDT library.
- The libast feature tests.

Change in the number of warnings on Linux when compiling with clang using
-Wsign-compare -Wshorten-64-to-32 -Wsign-conversion -Wimplicit-int-conversion:
'"${ printf "%'d => %'d => %'d" ${w[2]} ${w[3]} ${w[13]} ;}"' (progression from part 2 => part 3 => part 13)

Progresses https://github.com/ksh93/ksh/issues/592'


fetch src/lib/libast/regex/
fetch src/lib/libast/string/
fetch src/lib/libast/include/regex.h
fetch src/lib/libast/include/swap.h
fetch src/lib/libast/man/swap.3
fetch src/lib/libast/man/regex.3
upc
git commit -m 'size_t/ptrdiff_t transition part 4: libast regex and string sublibraries

This is the fourth of the thickfold patch series, which enables ksh93
to operate within a 64-bit address space.

The parts of ksh93 affected by this commit are:
- The libast string library.
- The libast regex engine.

Change in the number of warnings on Linux when compiling with clang using
-Wsign-compare -Wshorten-64-to-32 -Wsign-conversion -Wimplicit-int-conversion:
'"${ printf "%'d => %'d => %'d" ${w[3]} ${w[4]} ${w[13]} ;}"' (progression from part 3 => part 4 => part 13)

Progresses https://github.com/ksh93/ksh/issues/592'



fetch src/lib/libast/comp/
fetch src/lib/libast/misc/
fetch src/lib/libast/port/
fetch src/lib/libast/include/cmdarg.h
fetch src/lib/libast/include/glob.h
fetch src/lib/libast/features/api
fetch src/lib/libast/std/assert.h
fetch src/lib/libast/include/debug.h
upc
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
  despite being used as though it were static, so it was
  marked static to reflect actual usage.
  - typefix() was not extern, but was missing its static designation,
    so that has also been rectified.
- Removed cmdopen_20110505() to avoid bitrot.

Change in the number of warnings on Linux when compiling with clang using
-Wsign-compare -Wshorten-64-to-32 -Wsign-conversion -Wimplicit-int-conversion:
'"${ printf "%'d => %'d => %'d" ${w[4]} ${w[5]} ${w[13]} ;}"' (progression from part 4 => part 5 => part 13)

Progresses https://github.com/ksh93/ksh/issues/592'

fetch src/lib/libast
upc
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

Change in the number of warnings on Linux when compiling with clang using
-Wsign-compare -Wshorten-64-to-32 -Wsign-conversion -Wimplicit-int-conversion:
'"${ printf "%'d => %'d => %'d" ${w[5]} ${w[6]} ${w[13]} ;}"' (progression from part 5 => part 6 => part 13)

Progresses https://github.com/ksh93/ksh/issues/592'

fetch src/lib/libast/features/tty
fetch src/lib/libcmd
upc
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

Change in the number of warnings on Linux when compiling with clang using
-Wsign-compare -Wshorten-64-to-32 -Wsign-conversion -Wimplicit-int-conversion:
'"${ printf "%'d => %'d => %'d" ${w[6]} ${w[7]} ${w[13]} ;}"' (progression from part 6 => part 7 => part 13)

Progresses https://github.com/ksh93/ksh/issues/592'

fetch src/lib/libdll
fetch src/lib/libsum
fetch src/cmd/builtin
fetch src/cmd/INIT
upc
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

Change in the number of warnings on Linux when compiling with clang using
-Wsign-compare -Wshorten-64-to-32 -Wsign-conversion -Wimplicit-int-conversion:
'"${ printf "%'d => %'d => %'d" ${w[7]} ${w[8]} ${w[13]} ;}"' (progression from part 7 => part 8 => part 13)

Progresses https://github.com/ksh93/ksh/issues/592'

fetch src/cmd/ksh93/edit
fetch src/cmd/ksh93/include/edit.h
fetch src/cmd/ksh93/include/history.h
upc
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

Change in the number of warnings on Linux when compiling with clang using
-Wsign-compare -Wshorten-64-to-32 -Wsign-conversion -Wimplicit-int-conversion:
'"${ printf "%'d => %'d => %'d" ${w[8]} ${w[9]} ${w[13]} ;}"' (progression from part 8 => part 9 => part 13)

Progresses https://github.com/ksh93/ksh/issues/592'

fetch src/cmd/ksh93/bltins
fetch src/cmd/ksh93/include/builtins.h
upc
git commit -m $'size_t/ptrdiff_t transition part 10: ksh93 preeminent builtin commands

This is the tenth of the thickfold patch series, which enables ksh93
to operate within a 64-bit address space.

The parts of ksh93 affected by this commit are:
- The various primary ksh93 builtins located in the bltins folder.
  - print_namval() returns a value only used for boolean tests,
    so it doesn\'t need to return nv_size + 1 and thus a warning
    can be quashed.

Change in the number of warnings on Linux when compiling with clang using
-Wsign-compare -Wshorten-64-to-32 -Wsign-conversion -Wimplicit-int-conversion:
'"${ printf "%'d => %'d => %'d" ${w[9]} ${w[10]} ${w[13]} ;}"' (progression from part 9 => part 10 => part 13)

Progresses https://github.com/ksh93/ksh/issues/592'

fetch src/cmd/ksh93/sh/n*
fetch src/cmd/ksh93/include/n*
fetch src/cmd/ksh93/sh/array.c
fetch src/cmd/ksh93/sh/string.c
fetch src/cmd/ksh93/sh/waitevent.c
fetch src/cmd/ksh93/nval.3
upc
git commit -m "size_t/ptrdiff_t transition part 11: ksh93 nval

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
  - The NV_MINSZ and nv_namptr macros now use offsetof()
    for this purpose as well.
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
- Removed the unused and inessential attsize and attval struct
  members (this is a remnant of the A__z botch, re: f215b10a).

Change in the number of warnings on Linux when compiling with clang using
-Wsign-compare -Wshorten-64-to-32 -Wsign-conversion -Wimplicit-int-conversion:
${ printf "%'d => %'d => %'d" ${w[10]} ${w[11]} ${w[13]} ;} (progression from part 10 => part 11 => part 13)

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
upc
git commit -m "size_t/ptrdiff_t transition part 12: ksh93 lexing, parsing and subshells

This is the twelfth of the thickfold patch series, which enables ksh93
to operate within a 64-bit address space.

The parts of ksh93 affected by this commit are:
- The lexing and parsing components in lex.c, parse.c, fcin.c
  and trestore.c.
- Minor fixes for the virtual subshell mechanism.
- The code underlying shcomp(1), aka sh_tdump().
- A minor fix to a cast in sh_timeradd().

Change in the number of warnings on Linux when compiling with clang using
-Wsign-compare -Wshorten-64-to-32 -Wsign-conversion -Wimplicit-int-conversion:
${ printf "%'d => %'d => %'d" ${w[11]} ${w[12]} ${w[13]} ;} (progression from part 11 => part 12 => part 13)

Progresses https://github.com/ksh93/ksh/issues/592"


fetch src
sed -i '5i '${ printf '%(%Y-%0m-%0d)T\n' now ;}':\n\n- Ksh is now capable of allocating memory within a 64-bit address space.\n' NEWS
git add NEWS
upc --all
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
    underlying SFIO and POSIX functions return values of
    that type.
  - sh_sfeval(): Removed the set but not used ep->slen variable.
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

Change in the number of warnings on Linux when compiling with clang using
-Wsign-compare -Wshorten-64-to-32 -Wsign-conversion -Wimplicit-int-conversion:
${ printf "%'d => %'d" ${w[12]} ${w[13]} ;} (progression from part 12 => part 13)

Most of the warnings that remain are mere bitflag issues of tertiary importance.
I'll note that I have a patch that expands nvflags to uint64_t which helps
quash some more warnings. But that patch isn't a part of the thickfold series
and is intended for submission sometime after this (hopefully) gets merged.

Fixes https://github.com/ksh93/ksh/issues/592"

git format-patch -k dev..64bit-fixes-series
