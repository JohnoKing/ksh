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

# Get warnings for initial parts
git checkout 33e66eac603f8203bc3239b251417a02a3762946  # part 1
bld
git checkout fefe896e35a7904088037d39dba4fae313fc33ac  # part 2
bld
git checkout 1c4bf2c65f3858784721e6d5944b19b4b4d00fd4  # part 3
bld
git checkout 69aa97299350c007dbac6abee26ae38e3caa74a5  # part 4
bld
git checkout 0e518418942347a73e38e82f3a40bcc0d7051c54  # part 5
bld
git checkout 043b4e9b869b8bacc71bbf7b65c13366077a5047  # part 6
bld
git checkout 8a36bf89a01d9d52010b7a7de2703bffc9d2457c  # part 7
bld
git checkout fb8d8144aa60c7d063213faad22d9563336f70ab  # part 8
bld
git checkout a58ccad183b16e602944109d9198e8c38c100c00  # part 9
bld
git checkout 3b979933cfe74dc8c3764adc2e699066f1a10a54  # part 10
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
	# Run the script from the from ksh wiki
	if [[ $1 == --all ]]; then
		test -f ../update-copyright.ksh && ksh ../update-copyright.ksh
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

fetch src/cmd/ksh93/sh/n*
fetch src/cmd/ksh93/include/n*
fetch src/cmd/ksh93/sh/array.c
fetch src/cmd/ksh93/sh/string.c
fetch src/cmd/ksh93/sh/waitevent.c
fetch src/cmd/ksh93/nval.3
upc
git commit -m "64-bit transition part 11: ksh93 nval

This is the eleventh of the thickfold patch series, which enables ksh93
to operate within a 64-bit address space.

The parts of ksh93 affected by this commit are:
- Various pieces of code wrongly assuming char is signed
  (follow-up to https://github.com/ksh93/ksh/issues/962).
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
git commit -m "64-bit transition part 12: ksh93 lexing, parsing and subshells

This is the twelfth of the thickfold patch series, which enables ksh93
to operate within a 64-bit address space.

The parts of ksh93 affected by this commit are:
- The lexing and parsing components in lex.c, parse.c, fcin.c
  and trestore.c.
  - Added an assert() after stkalloc() to fix the following
    LTO warning:
    warning: 'memset' writing 60 bytes into a region of size 0 overflows the destination [-Wstringop-overflow=]
      1819 |                         memset(ioq,0,sizeof(*ioq));
           |                         ^
    lto1: note: destination object is likely at address zero
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
git commit -m "64-bit transition part 13: the rest of ksh93

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
