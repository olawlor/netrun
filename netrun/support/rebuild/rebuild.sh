#!/bin/sh
#
# Connect to each netrun box, and rebuild the cached main.obj
#

tar cvfh rebuild_main.tar project || exit 1

#for server in 137.229.25.138 ppc2 itanium1 poweredge sgi1 dec1 orlando viz1 powerwall6

for server in olawlor viz1 sandy phenom atomic ppc2 poweredge sgi1 orlando viz1 powerwall6
do
	echo "--------- $server ----------"
	../../bin/sandsend -f rebuild_main.tar $server:2983
done

echo "--------- x86 64-bit ----------"
../../bin/sandsend -f rebuild_main.tar olawlor:9923

echo "--------- PowerPC ----------"
../../bin/sandsend -f rebuild_main.tar olawlor:9933

echo "--------- Windows 32-bit ----------"
echo 'LINKER=cl /nologo /EHsc /DWIN32=1 ' >> project/Makefile
tar cvfh rebuild_main.tar project || exit 1
../../bin/sandsend -f rebuild_main.tar olawlor:9943
cp project/Makefile.orig project/Makefile
