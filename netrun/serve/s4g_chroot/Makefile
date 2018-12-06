# Makefile for s4g_chroot
#  This Makefile must be run by root.

# Typical usage:
# FLAGS='-DlibDir="foobar"'

all: s4g_chroot

s4g_chroot: main.c
	gcc $(FLAGS) $< -Wall -o $@
	chmod 4111 $@

install: s4g_chroot
	cp $< /usr/local/bin/
	g++ -fopenmp -lpthread copy_libs.cpp -o copy_libtest
	./copy_libs.sh ./copy_libtest
	$< ./copy_libtest

caps:
	setcap cap_setgid,cap_setuid,cap_sys_chroot+ep /usr/local/bin/s4g_chroot 

clean:
	- rm s4g_chroot

