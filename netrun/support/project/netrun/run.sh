#!/bin/sh
#
#  Usage: run.sh <description> <source file> <command & arguments>
# Run this command, preparing the output for HTML display.
#

desc="$1"
src="$2"
shift; shift

# echo "<hr><h1>$desc</h1>"
#echo "Executing $desc: $@ <br>"
#echo "$desc: $@ <br>"

# Run command, capturing stdout and stderr to "out" file.
"$@" > out 2>&1

res=$?

# If there's anything in the file, show output of command
if [ -s out ]
then
	echo "Executing $desc: $@ <br>"
	
	color="#CCCCCC"
	[ $res -ne 0 ] && color="#FF8888"

	echo '<TABLE><TR><TD BGCOLOR="'$color'">
<! @NetRun/'$desc'@ begin >'
#  Sadly, -c fails on IRIX:
#	cat out | head -c 20000 | head -n 200 > out_lil
	cat out | head -n 1000 > out_lil
	mv out_lil out
	cat out | netrun/filter_htmlpre.pl
	echo '<! @NetRun/'$desc'@ end >
</TD></TR></TABLE>'
fi

# Show source code (with line numbers) if something went wrong
if [ "$res" -ne 0 -a ! -z "$src" ]
then
	echo "Error $res during $desc: "
	
	echo '<TABLE><TR><TD VALIGN=top><PRE>'
	cat $src | netrun/filter_lineno.pl
	echo '</PRE></TD><TD VALIGN=top BGCOLOR="'$color'">'
	cat $src | netrun/filter_htmlpre.pl
	echo '</TD></TR></TABLE>'
fi

exit $res
