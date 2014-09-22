/**
 Main routine for NetRun.

 Orion Sky Lawlor, olawlor@acm.org, 2005/09/14 (Public Domain)
*/
#include <stdio.h>
#include <stdlib.h>
#include "inc.h"  /* Utility declarations and routines */
#include "inc.c"  /* Just include implementation of utility routines here... */
#include "signals.c"

#if defined(__GNUC__) && (__GNUC__<3)
/* Special simple version for primitive compiler on ancient MIPS machine */
void netrun_call( int (*foofn)(void) ) {
	int value=foofn();
	printf("Program complete.  Return %d (0x%X)\n",value,value);
}

bool netrun_data_readable(void) { return false; }

#else /* normal C++ operator overloaded version */
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

int trash_eax(void); // trashes return value register

/* The overloaded netrun_call wrappers figure out if there's an argument or return type,
  and generate the argument and/or handle the return type. */
template <typename R,typename A>
int netrun_call( R (*foofn)(A) ) {
	register A arg=netrun_get_arg(A());
	register int e=trash_eax();
	netrun_handle_ret(foofn(arg));
	return e;
}

template <typename R>
void netrun_call( R (*foofn)() ) {
	netrun_handle_ret(foofn());
}

template <typename A>
int netrun_call( void (*foofn)(A) ) {
	register A arg=netrun_get_arg(A());
	register int e=trash_eax();
	foofn(arg);
	return e;
}

void netrun_call( void (*foofn)() ) {
	foofn();
}

/** This is designed to keep calling our input-consuming code until EOF */
bool netrun_data_readable(void) {
	std::istream &is=std::cin;
	static long last_whitespace=1;
	if (!is) return false; // EOF bit set (stream is done)
	
	long this_whitespace=0;
	// Skip whitespace, while checking for EOF
	while (true) {
		int c=is.get(); // read character
		if (c==EOF) return false; // no data left
		else if (c=='\r' || c=='\n' || c==' ' || c=='\t') {
			this_whitespace++;
			continue;
		}
		else // a real character--throw it back and leave.
		{ 
			is.putback((char)c);  
			break; 
		}
	}
	
	if (this_whitespace==0 && last_whitespace==0) return false; // didn't read anything
	last_whitespace=this_whitespace;

	return true;
}
#endif

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

	} while (netrun_data_readable());
	
	exit(0);
}

int trash_eax() { return 0xf00123; }

