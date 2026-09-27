#!/bin/sh

while true
do
wget -q -O key_file $url'?src_name='$src_name'&cmd=get_job' 2>get_job.log
key=`cat key_file`
echo $key
if [ "$key" = "" ]; then
exit 1
fi

../../solver $key >solve_content 2>>solver.log
wget -q -O result_file --post-file=solve_content $url'?src_name='$src_name'&cmd=save_job&key='$key'&pc='$process_count 2>save_job.log
done
