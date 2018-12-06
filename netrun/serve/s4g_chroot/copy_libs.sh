#!/bin/sh
#  Copy all shared libraries needed by this exe into /usr/local/bin/s4g_libs
d="/usr/local/bin/s4g_libs"
mkdir "$d"

ldd "$1" | (
	while read soname arrow srcname addr
	do
		soname=`echo "$soname" | awk -F/ '{print $NF}'`
		echo "$soname  comes from  $srcname "
		cp "$srcname" "$d/$soname"
	done
) 

echo "Copied over all of the above libs."
