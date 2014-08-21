#!/bin/sh
# Use gdb to kill off "TN" processes in traced state.
#  Without this script, even "kill -9" doesn't work...

while [ $# -gt 0 ]
do
	pid=$1; shift;
	echo "$pid"
	gdb -p $pid << EOF
kill
quit
EOF
	kill -s 0 $pid
done
