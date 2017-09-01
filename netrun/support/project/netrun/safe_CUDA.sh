#!/bin/sh
#  Usage: safe_CUDA.sh  <application>  <args>
export DISPLAY=:0
mkdir -p run
cp "$1" run/code.exe
shift
cd run
exec /usr/local/bin/s4g_chrootssh ./code.exe "$@"
#exec /usr/local/bin/s4g_chrootssh /bin/strace ./code.exe "$@"

