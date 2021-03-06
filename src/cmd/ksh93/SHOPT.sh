#
# Compile-time SHOPT_* options for ksh93.
# 1 to enable, 0 to disable, empty value to probe.
#

SHOPT 2DMATCH=1				# two dimensional ${.sh.match} for ${var//pat/str}
SHOPT ACCT=				# accounting
SHOPT ACCTFILE=				# per-user accounting info
SHOPT AUDIT=1				# enable auditing per SHOPT_AUDITFILE
SHOPT AUDITFILE='\"/etc/ksh_audit\"'	# auditing file
SHOPT BGX=1				# one SIGCHLD trap per completed job
SHOPT BRACEPAT=1			# C-shell {...,...} expansions (, required)
SHOPT CMDLIB_HDR=  # '<cmdlist.h>'	# custom -lcmd list for path-bound builtins
SHOPT CMDLIB_DIR=  # '\"/opt/ast/bin\"'	# virtual directory prefix for path-bound builtins
SHOPT CRNL=				# accept MS Windows newlines (<cr><nl>) for <nl>
SHOPT DYNAMIC=1				# dynamic loading for builtins
SHOPT ECHOPRINT=			# make echo equivalent to print
SHOPT EDPREDICT=1			# predictive editing
SHOPT ESH=1				# emacs/gmacs edit mode
SHOPT FILESCAN=1			# fast file scan
SHOPT FIXEDARRAY=1			# fixed dimension indexed array
SHOPT HISTEXPAND=1			# csh-style history file expansions
SHOPT KIA=				# ksh -R <outfile> <script> generates cross-ref database from script
SHOPT MULTIBYTE=1			# multibyte character handling
SHOPT NAMESPACE=1			# allow namespaces
SHOPT OLDTERMIO=			# support both TCGETA and TCGETS
SHOPT OPTIMIZE=1			# optimize loop invariants
SHOPT PFSH=0				# solaris exec_attr(4) profile execution (obsolete)
SHOPT P_SUID=				# real uid's that require -p for set[ug]id (do not set to 0 to turn off)
SHOPT RAWONLY=1				# make viraw the only vi mode
SHOPT REGRESS=				# enable __regress__ builtin and instrumented intercepts for testing
SHOPT REMOTE=				# enable --rc if running as a remote shell
SHOPT SEVENBIT=				# strip the eighth bit from characters
SHOPT SPAWN=				# use spawnveg for fork/exec
SHOPT STATS=1				# add .sh.stats variable
SHOPT SUID_EXEC=1			# allow (safe) suid/sgid shell scripts
SHOPT SYSRC=				# attempt . /etc/ksh.kshrc if interactive
SHOPT TIMEOUT=				# number of seconds for shell timeout
SHOPT TYPEDEF=1				# enable typeset type definitions
SHOPT VSH=1				# vi edit mode
