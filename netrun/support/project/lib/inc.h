/**
  Weird assorted helpful routines for debugging and 
  testing assembly code.
  
  Orion Sky Lawlor, olawlor@acm.org, 2005/09/14 (Public Domain)
*/
#ifndef __OSL_NETRUN_INC_H /* prevent repeated inclusion */
#define __OSL_NETRUN_INC_H

#include <stdlib.h>

#ifdef __cplusplus /* C++ uses special name mangling (not in C or Assembly) */
#  define CDECL extern "C"
#include <stdexcept>

/* C++ only routines */
/* 2007 CS321 HW6.2 */
inline int netrun_access_cool_address(void) { 
	int *p=(int *)0x1986000; (*p)++; return (int)(long)p; 
}
inline void *netrun_allocate_mystery_size(void) {
	const char *mys=getenv("MYSTERY"); if (mys==0) mys="3840";
	return malloc(atoi(mys));
}


#ifdef __SSE__
#include <xmmintrin.h>
#include <iostream>

class sse_float4 {
public:
	__m128 v;
	sse_float4(__m128 v_) :v(v_) {}

	friend std::ostream & operator<<(std::ostream &os,const sse_float4 &s) {
		float f[4];
		_mm_store_ps(f,s.v);
		std::cout<<"{"<<f[0]<<","<<f[1]<<","<<f[2]<<","<<f[3]<<"}";
		return os;
	}
};

class sse_double2 {
public:
	__m128d v;
	sse_double2(__m128d v_) :v(v_) {}

	friend std::ostream & operator<<(std::ostream &os,const sse_double2 &s) {
		double d[2];
		_mm_store_pd(d,s.v);
		std::cout<<"{"<<d[0]<<","<<d[1]<<"}";
		return os;
	}
};


#endif

#else
#  define CDECL extern /* empty-- already C or Assembly */
#endif

/**
 Function prototype for user-written subroutine "foo" (C, C++, Fortran77, or Assembly) 
*/
#ifndef NETRUN_FOO_DECL
#define NETRUN_FOO_DECL int foo(void)
#endif
CDECL NETRUN_FOO_DECL;

/* Print data in memory at this pointer as raw bits. */
CDECL void dump_binary(const void *ptr,long nbits);
/* Print data in memory at this pointer as raw hex digits. */
CDECL void dump_hex(const void *ptr,long nbits);
/* Print data in memory at this pointer as raw chars. */
CDECL void dump_ascii(const void *ptr,long nbits);


/**
  Read one input integer from the user. 
  WARNING: When called from assembly, may trash registers!
*/
CDECL long read_input(void);
CDECL float read_float(void);

/**
  Read input line from the user.  Returns 1 if read OK, 0 if error or EOF.
  The string must be at least 100 characters long; longer inputs
  are broken into pieces.
*/
CDECL int read_string(char *dest);

/**
  Print this integer to the screen.  May trash registers.
*/
CDECL void print_int(int i);
CDECL void print_long(long i);
CDECL void print_float(float f);

/**
  Print the stack to the screen.  Does NOT trash registers.
*/
CDECL void print_stack(void);


/******* Function Performance Profiling ***********/
extern int timer_only_dont_print;

typedef int (*timeable_fn)(void);

/**
  Return the current time in seconds (since something or other).
*/
CDECL double time_in_seconds(void);

/**
  Return the number of seconds this function takes to run.
  May run the function several times (to average out 
  timer granularity).
*/
CDECL double time_function(timeable_fn fn);

/**
  Time a function's execution, and print this time out.
*/
CDECL void print_time(const char *fnName,timeable_fn fn);

/********* Checksums ***************/
CDECL int iarray_print(int *arr,int n);
CDECL long larray_print(long *arr,long n);
CDECL int farray_print(float *arr,int n);
CDECL void farray_fill(float *f,int n,float tol);
CDECL void farray_fill2(float *f,int n,float tol);
CDECL int farray_checksum(float *f,int n,float tol);

/* Print the contents of this file */
CDECL void cat(const char *fileName);




#endif
