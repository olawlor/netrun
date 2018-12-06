#!/bin/sh
#
# Fork off a subshell that
# runs sandserv server in a loop, keeping a log of its actions.
#

cd `dirname $0`
d=`pwd`
cd $d/run
log="$d/sandserv.log"
echo "sandserv.sh starting: "`date`" in $d" >> $log 
while [ true ]
do
	echo "sandserv.exe starting: "`date` >> $log 
	echo "$d/sandserv" | su netrun >> $log 2>&1
	echo "SECURITY: sandserv killed off somehow" >> $log
	sleep 10
	echo kill -9 -1 | su netrun
	# Kill *everything* ending in .exe:
	$d/kill_gdb.sh
done
