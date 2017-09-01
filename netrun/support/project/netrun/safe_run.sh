#!/bin/sh
# New version, calling "s4g_chroot" which builds "run" directory,
#  enforces time limit, and enforces process limit.
# This is faster (no perl), more reliable (no zombies), and cleaner than below.
if [ ! -r run/proc/stat ]
then
  mkdir -p run/proc
  if [ -r /proc/stat ]
  then
	cp /proc/stat run/proc/stat
  else
	echo -e "cpu\ncpu0\ncpu1" > run/proc/stat
  fi
fi
mkdir -p run/etc
cp /etc/resolv.conf /etc/nsswitch.conf /etc/services /etc/hosts /etc/host.conf run/etc/  > /dev/null 2>&1
mkdir -p run/dev
dd if=/dev/urandom of=run/dev/urandom bs=4k count=1 > /dev/null 2>&1

exec /usr/local/bin/s4g_chroot "$@"
