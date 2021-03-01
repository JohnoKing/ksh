# These are regression tests for the local and declare builtins.
# local and declare are called with 'command' because in ksh93v-
# and ksh2020 they may be special builtins.

function err_exit
{
	print -u2 -n "\t"
	print -u2 -r ${Command}[$1]: "${@:2}"
	(( Errors+=1 ))
}
alias err_exit='err_exit $LINENO'

Command=${0##*/}
integer Errors=0

# ======
# This test must be run first due to the next test.
command local 2> /dev/null && err_exit '`local` works outside of functions'

# local shouldn't suddenly work outside of functions after a POSIX function runs local.
dummy() { command local; }
dummy && command local 2> /dev/null && err_exit 'the local builtin works outside of functions after a POSIX function runs local'

# ======
# The local builtin shouldn't have output when there aren't any arguments (as in Bash).
function test_local_output { [[ $(local) ]] && err_exit '`local` has output when no arguments are given'; }
test_local_output

# ======
for i in declare local; do
	# local should work inside both kinds of functions, without reliance on environment variables.
	function ksh_function_nounset {
		command $i foo=bar 2>&1
	}
	function ksh_function_unset {
		unset .sh.fun
		command $i foo=bar 2>&1
	}
	posix_function_nounset() {
		command $i foo=bar 2>&1
	}
	posix_function_unset() {
		unset .sh.fun
		command $i foo=bar 2>&1
	}
	[[ $(ksh_function_nounset) ]] && err_exit "'$i' fails inside of KornShell functions"
	[[ $(ksh_function_unset) ]] && err_exit "'$i' fails inside of KornShell functions when \${.sh.fun} is unset"
	[[ $(posix_function_nounset) ]] && err_exit "'$i' fails inside of POSIX functions"
	[[ $(posix_function_unset) ]] && err_exit "'$i' fails inside of POSIX functions when \${.sh.fun} is unset"

	# The local and declare builtins should have a dynamic scope
	# Tests for the scope of POSIX functions
	foo=globalscope
	subfunc() {
		[[ $foo == dynscope ]]
	}
	mainfunc() {
		command $i foo=dynscope
		subfunc
	}
	mainfunc || err_exit "'$i' is not using a dynamic scope in POSIX functions"
	# TODO: `local` shouldn't change global variables outside of the function's scope
	#[[ $val == globalscope ]] || err_exit "'$i' changes variables outside of a POSIX function's scope"

	# Tests for the scope of KornShell functions
	bar=globalscope
	function subfunc_b {
		[[ $bar == dynscope ]]
	}
	function mainfunc_b {
		command $i bar=dynscope
		subfunc_b
	}
	mainfunc_b || err_exit "'$i' is not using a dynamic scope in KornShell functions"
	[[ $bar == globalscope ]] || err_exit "'$i' changes variables outside of a KornShell function's scope"
done

# The declare builtin should work outside of functions
unset foo
declare foo=bar
[[ $foo == bar ]] || err_exit "'declare' doesn't work outside of functions"

# ======
exit $((Errors<125?Errors:125))
