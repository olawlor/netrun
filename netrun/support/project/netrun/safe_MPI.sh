#!/bin/sh
#  Usage: safe_MPI.sh <num cores>  <application>  <args>
num="$1"
shift
mkdir -p run
cp "$1" run/code.exe
shift
cd run
exec mpirun -np $num /usr/local/bin/s4g_chrootssh code.exe "$@"
