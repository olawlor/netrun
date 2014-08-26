/**
 Main routine for NetRun.

 Orion Sky Lawlor, olawlor@acm.org, 2005/09/14 (Public Domain)
*/
#include <stdio.h>
#include <stdlib.h>
#include "inc.h"  /* Utility declarations and routines */
#include "inc.c"  /* Just include implementation of utility routines here... */
#include "signals.c"

#include <iostream>

/* These overloaded functions generate foo's arguments. */
template <typename T>
T netrun_get_arg(T dummy) {
	T r=T();
	std::cin>>r;
	return r;
}
long netrun_get_arg(long dummy) { return read_input(); }
int  netrun_get_arg(int  dummy) { return read_input(); }

/* These overloaded functions handle foo's return type. */
template <typename T>
void netrun_handle_ret(T value) {
	std::cout<<"Program complete.  Return '"<<value<<"'\n";
}
void netrun_handle_ret(int value) {
	printf("Program complete.  Return %d (0x%X)\n",value,value);
}
void netrun_handle_ret(long value) {
	printf("Program complete.  Return %ld (0x%lX)\n",value,value);
}

/* The overloaded netrun_call wrappers figure out if there's an argument or return type,
  and generate the argument and/or handle the return type. */
template <typename R,typename A>
void netrun_call( R (*foofn)(A) ) {
	netrun_handle_ret(foofn(netrun_get_arg(A())));
}

template <typename R>
void netrun_call( R (*foofn)() ) {
	netrun_handle_ret(foofn());
}

template <typename A>
void netrun_call( void (*foofn)(A) ) {
	foofn(netrun_get_arg(A()));
}

void netrun_call( void (*foofn)() ) {
	foofn();
}

/** This is designed to keep calling our input-consuming code until EOF */
bool netrun_data_readable(std::istream &is) {
	static std::streampos last_pos=0u;
	if (!is) return false; // EOF bit set (stream is done)
	if (is.tellg()==last_pos) return false; // didn't read any data last time
	
	// Skip whitespace, while checking for EOF
	int c=EOF;
	while (c==is.get()) { // read character
		if (c==EOF) return false; // no data left
		if (c=='\r' || c=='\n' || c==' ' || c=='\t') continue;
		else { is.putback((char)c);  break; }
	}
	
	last_pos=is.tellg(); // store position of real character
	return true; // data is readable
}


/* This junk is used to detect modifications to preserved registers: */
int g0=0xf00d00, g1=0xf00d01, g2=0xf00d02, g3=0xf00d03, g4=0xf00d04, g5=0xf00d05, g6=0xf00d06, g7=0xf00d07;

int main() {
	int local=0x1776; /* stored on stack */
	register long l0=g0,l1=g1,l2=g2,l3=g3,l4=g4,l5=g5,l6=g6,l7=g7; /* stored in registers */
	handle_signals();
	
#ifdef TIME_FOO /* Timing mode */
	printf("Timing foo...\n");
	print_time("foo",(timeable_fn)foo);
#endif

/* Normal case */
	do {
		netrun_call(&foo);

	if (local!=0x1776) printf("ERROR! Main's stack space was overwritten!\n");
	if (l0!=g0||l1!=g1||l2!=g2||l3!=g3||l4!=g4||l5!=g5||l6!=g6||l7!=g7) printf("ERROR! Main's local variables changed--\n"
"Either some preserved registers were overwritten,\n"
"or else part of the stack was changed or overwritten!\n");

	} while (netrun_data_readable(std::cin));
	
	exit(0);
}
