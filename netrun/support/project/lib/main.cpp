/**
 Main routine for NetRun.

 Orion Sky Lawlor, olawlor@acm.org, 2005/09/14 (Public Domain)
*/
#include <stdio.h>
#include <stdlib.h>
#include "inc.h"  /* Utility declarations and routines */
#include "inc.c"  /* Just include implementation of utility routines here... */
#include "signals.c"

/* This junk is used to detect modifications to preserved registers: */
int g0=0xf00d00, g1=0xf00d01, g2=0xf00d02, g3=0xf00d03, g4=0xf00d04, g5=0xf00d05, g6=0xf00d06, g7=0xf00d07;

int main() {
	int v,local=0x1776; /* stored on stack */
	register long l0=g0,l1=g1,l2=g2,l3=g3,l4=g4,l5=g5,l6=g6,l7=g7; /* stored in registers */
	handle_signals();
	
#ifdef TIME_FOO /* Timing mode */
	printf("Timing foo subroutine...\n");
	print_time("foo",foo);
#endif
/* Normal case */
	v=foo();
	printf("Program complete.  Return %d (0x%X)\n",v,v);
	if (local!=0x1776) printf("ERROR! Main's stack space was overwritten!\n");
	if (l0!=g0||l1!=g1||l2!=g2||l3!=g3||l4!=g4||l5!=g5||l6!=g6||l7!=g7) printf("ERROR! Main's local variables changed--\n"
"Either some preserved registers were overwritten,\n"
"or else part of the stack was changed or overwritten.\n");
	exit(0);
}
