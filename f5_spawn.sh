#!/bin/sh
export src_name=somebody
#export common_deep=2
#export threat_deep=12
export prove_mode=1

cur_dir=`pwd`

daemon --chdir=$cur_dir --pidfile=$cur_dir/pid --stdout=$cur_dir/spawn_out.log --stderr=$cur_dir/spawn_err.log python solver_web.py
