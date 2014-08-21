#!/bin/sh
if [ -x "$1" ]
then
	"$@"
fi
exit 0

