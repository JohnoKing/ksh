########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2011 AT&T Intellectual Property          #
#          Copyright (c) 2020-2022 Contributors to ksh 93u+m           #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 2.0                  #
#                                                                      #
#                A copy of the License is available at                 #
#      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      #
#         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         #
#                                                                      #
#                  David Korn <dgk@research.att.com>                   #
#                  Martijn Dekker <martijn@inlv.org>                   #
#                                                                      #
########################################################################

. "${SHTESTS_COMMON:-${0%/*}/_common}"

typeset -T Pt_t=(
	float x=1
	float y=0
	len()
	{
		print -r $((sqrt(_.x*_.x + _.y*_.y)))
	}
)

typeset -T Rect_t=(
	Pt_t ll=(x=0 y=0)
	Pt_t ur=(x=1 y=1)
	area()
	{
		print -r $(( abs((_.ur.x-_.ll.x)*(_.ur.y-_.ll.y)) ))
	}
)

for ((i=0; i < 100; i++))
do
Rect_t r
[[ ${r.area} == 1 ]] || err_exit '${r.area} != 1'
Rect_t s=(
	Pt_t ur=(x=9 y=9)
	Pt_t ll=(x=7 y=7)
)
[[ ${s.ur.x} == 9 ]] || err_exit ' ${s.ur.x} != 9'
(( s.ur.x == 9  ))|| err_exit ' ((s.ur.x)) != 9'
[[ ${s.ll.y} == 7 ]] || err_exit '${s.ll.y} != 7'
(( s.area == 4 )) || err_exit  'area of s should be 4'
[[ ${s.area} == 4 ]] || err_exit '${s.area} != 4'
unset r s
done
Rect_t -A r
r[one]=(ur=(x=4 y=4))
(( r[one].area == 16 )) || err_exit 'area of r[one] should be 16'
[[ ${r[one].area} == 16 ]] || err_exit '${r[one].area} should be 16'
unset r

exit $((Errors<125?Errors:125))
