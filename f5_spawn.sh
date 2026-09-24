#!/bin/sh
url=http://93.127.143.124/solutions/root/solve.php
src_name=somebody
process_count=`cat /proc/cpuinfo | grep processor | wc -l`
#set stored_deep=2
#set threat_deep=8
prove_mode=1

export process_count
export url
export src_name
#export stored_deep
#export threat_deep
export prove_mode

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
