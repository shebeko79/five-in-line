#!/bin/sh
export url=http://93.127.143.124/solution/solve.php
export src_name=somebody
export process_count=`nproc`
#export common_deep=2
export threat_deep=12
export prove_mode=1

cur_dir=`pwd`

c=0
mkdir spawn
while [ $c -lt $process_count ]
do
mkdir ./spawn/$c
cd ./spawn/$c
daemon --chdir=$cur_dir/spawn/$c --pidfile=$cur_dir/spawn/$c/pid ../../f5_solve.sh
cd ../../
c=`expr $c + 1`
done
