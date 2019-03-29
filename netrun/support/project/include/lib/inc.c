/**
  Weird assorted helpful include files for debugging and 
  testing assembly code.
  
  Orion Sky Lawlor, olawlor@acm.org, 2005/09/14 (Public Domain)
*/
#include <stdio.h>
#include <stdlib.h>
#include <algorithm> /* for std::sort */
#include "inc.h"

/* To avoid cluttering up the screen during timing tests... */
int timer_only_dont_print=0;


/* Read one input integer from the user. */
long read_input(void) {
	long ret=0;

	if (timer_only_dont_print) return 0;
	printf("Please enter an input value:\n");
	fflush(stdout);
	if (1!=scanf("%li",&ret)) {
		if (feof(stdin))
			printf("read_input> No input to read!  Exiting...\n");
		else
			printf("read_input> Invalid input format!  Exiting...\n"); 
		exit(1);
	}
	if (ret<0) /* 32-bit output, for backward compatibility */
		printf("read_input> Returning %ld (0x%X)\n",ret,(int)ret);
	else
		printf("read_input> Returning %ld (0x%lX)\n",ret,ret);
	return ret;
}


/* Read one input float from the user. */
float read_float(void) {
	float ret=0;

	if (timer_only_dont_print) return 0;
	printf("Please enter a float input value:\n");
	fflush(stdout);
	if (1!=scanf("%f",&ret)) {
		if (feof(stdin))
			printf("read_float> No input to read!  Exiting...\n");
		else
			printf("read_float> Invalid input format!  Exiting...\n"); 
		exit(1);
	}
	printf("read_float> Returning %f\n",ret);
	return ret;
}

/* Read one input string from the user. Returns 0 if no more input ready. */
int read_string(char *dest_str) {
	if (0==fgets(dest_str,100,stdin)) {
		return 0;
	}
	dest_str[99]=0;
	return 1;
}

/* Print this integer parameter (on the stack) */
void print_int(int i) {
	if (timer_only_dont_print) return;
	printf("Printing integer %d (0x%X)\n",i,i);
}
void print_long(long i) {
	if (timer_only_dont_print) return;
	printf("Printing integer %ld (0x%lX)\n",i,i);
}
void print_float(float f) {
	if (timer_only_dont_print) return;
	printf("Printing float %f (%g)\n",f,f);
}
CDECL void print_int_(int *i) {print_int(*i);} /* fortran, gfortran compiler */
CDECL void print_int__(int *i) {print_int(*i);} /* fortran, g77 compiler */

/******* Function Performance Profiling ***********/
/**
  Return the current time in seconds (since something or other).
*/
#if defined(_WIN32)
#  include <sys/timeb.h>
#  define time_in_seconds_granularity 0.1 /* seconds */
double time_in_seconds(void) { /* This seems to give terrible resolution (60ms!) */
        struct _timeb t;
        _ftime(&t);
        return t.millitm*1.0e-3+t.time*1.0;
}
#else /* UNIX or other system */
#  include <sys/time.h> //For gettimeofday time implementation
#  define time_in_seconds_granularity 0.001 /* seconds */
double time_in_seconds(void) {
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return tv.tv_usec*1.0e-6+tv.tv_sec*1.0;
}
#endif

/* A little empty subroutine, just to measure call/return overhead 
  inside time_fn, below. */
CDECL int timeable_fn_empty(void) {
	return 0;
}

/**
  Return the number of seconds this function takes to run.
  May run the function several times (to average out 
  timer granularity).
*/
double time_function_onepass(timeable_fn fn)
{
	unsigned int i,count=1;
	double timePer=0;
	for (count=1;count!=0;count*=2) {
		double start, end, elapsed;
		timer_only_dont_print=1;
		start=time_in_seconds();
		for (i=0;i<count;i++) fn();
		end=time_in_seconds();
		timer_only_dont_print=0;
		elapsed=end-start;
		timePer=elapsed/count;
		if (elapsed>time_in_seconds_granularity) /* Took long enough */
			return timePer;
	}
	/* woa-- if we got here, "count" reached integer wraparound before 
	  the timer ran long enough.  Return the last known time */
	return timePer;
}

/**
  Return the number of seconds this function takes to run.
  May run the function several times (to average out 
  timer granularity).
*/
double time_function(timeable_fn fn)
{
	static double empty_time=-1;
	if (empty_time<0) { /* Estimate overhead of subroutine call alone */
		empty_time=0; /* To avoid infinite recursion! */
		/* empty_time=time_function(timeable_fn_empty); */
	}
	enum {
#if defined(_WIN32) /* Win32 timer has coarse granularity--too slow otherwise! */
		ntimes=1
#else
		ntimes=3
#endif
	};
	double times[ntimes];
	for (int t=0;t<ntimes;t++)
		times[t]=time_function_onepass(fn);
	std::sort(&times[0],&times[ntimes]);
	return times[ntimes/2]-empty_time;
}

/**
  Time a function's execution, and print this time out.
*/
void print_time(const char *fnName,timeable_fn fn)
{
	double sec=time_function(fn);
	printf("%s: ",fnName);
	if (1 || sec<1.0e-6) printf("%.2f ns/call\n",sec*1.0e9);
	else if (sec<1.0e-3) printf("%.2f us/call\n",sec*1.0e6);
	else if (sec<1.0e0) printf("%.2f ms/call\n",sec*1.0e3);
	else printf("%.2f s/call\n",sec);
}

/********* Checksums ***************/
int iarray_print(int *arr,int n)
{
	int i=0,p;
	if (n<0 || n>1000000) {
		printf("ERROR in iarray_print: passed invalid number of elements %d (0x%08x) for array %p.  Did you pass the arguments in the wrong order?\n",
			n,n,arr);
		exit(1);
	}
	if (timer_only_dont_print) return n;
	p=n;
	if (p>10) p=10; /* Only print first 10 elements */
	printf("iarray_print: %d elements\n",n);
	for (i=0;i<p;i++)
		printf("  arr[%d]=%d  (0x%08x)\n",i,arr[i],arr[i]);
	if (p<n) {
		i=n/2;
		printf("  arr[%d]=%d  (0x%08x)\n",i,arr[i],arr[i]);
		i=n-1;
		printf("  arr[%d]=%d  (0x%08x)\n",i,arr[i],arr[i]);
	}
	return n;
}

long larray_print(long *arr,long n)
{
	long i=0,p;
	if (n<0 || n>1000000) {
		printf("ERROR in larray_print: passed invalid number of elements %ld (0x%08lx) for array %p.  Did you pass the arguments in the wrong order?\n",
			n,n,arr);
		exit(1);
	}
	if (timer_only_dont_print) return n;
	p=n;
	if (p>10) p=10; /* Only print first 10 elements */
	printf("larray_print: %ld elements\n",n);
	for (i=0;i<p;i++)
		printf("  arr[%ld]=%ld  (0x%08lx)\n",i,arr[i],arr[i]);
	if (p<n) {
		i=n/2;
		printf("  arr[%ld]=%ld  (0x%08lx)\n",i,arr[i],arr[i]);
		i=n-1;
		printf("  arr[%ld]=%ld  (0x%08lx)\n",i,arr[i],arr[i]);
	}
	return n;
}

int farray_print(float *arr,int n)
{
	int i=0,p;
	if (n<0 || n>1000000) {
		printf("ERROR in farray_print: passed invalid number of elements %d (0x%08x) for array %p.  Did you pass the arguments in the wrong order?\n",
			n,n,arr);
		exit(1);
	}
	if (timer_only_dont_print) return n;
	p=n;
	if (p>10) p=10; /* Only print first 10 elements */
	printf("farray_print: %d elements\n",n);
	for (i=0;i<p;i++)
		printf("  arr[%d]=%f\n",i,arr[i]);
	return n;
}
void farray_fill(float *f,int n,float tol)
{
	int i,v=1;
	for (i=0;i<n;i++) {
		f[i]=(v&0xff)*tol;
		v=v*3+14;
	}
	f[n]=1776.0; /* sentinal value */
}
void farray_fill2(float *f,int n,float tol)
{
	int i,v=0xbadf00d;
	for (i=0;i<n;i++) {
		f[i]=(((v>>8)&0xff)+1)*tol;
		v=v*69069+1;
	}
	f[n]=1776.0; /* sentinal value */
}
int farray_checksum(float *f,int n,float tol)
{
	int i,v=0;
	double t=1.0/tol;
	if (f[n]!=1776.0) {
		printf("ERROR!  You wrote past the end of the array!\n");
		exit(0);
	}
	for (i=0;i<n;i++) {
		int k=(int)(f[i]*t);
		v+=k*(i+10+v);
		v&=0xffff;
	}
	printf("First 5 values: %d %d %d %d %d\n",
		(int)(f[0]*t),(int)(f[1]*t),(int)(f[2]*t),(int)(f[3]*t),(int)(f[4]*t));
	return v;
}

void cat(const char *fName) {
	FILE *f=fopen(fName,"rb");
	int binary=0, len=0, i;
	if (f==0) { printf("File '%s' does not exist.\n",fName); return; }
	fseek(f,0,SEEK_END);
	len=ftell(f);
	fseek(f,0,SEEK_SET);
	printf("-- %s: %d bytes --\n",fName,len);
	if (len>1000) len=1000;
	for (i=0;i<len;i++) {
		unsigned char c=fgetc(f);
		if (c<9 || c>127) binary=1;
		if (binary) {
			if (i%16 == 0) printf("\n");
			printf("0x%02x ",c);
		} else {
			printf("%c",c);
		}
	}
	printf("-- end of %s --\n",fName);
	fclose(f);
}

/* Print a byte in binary */
void print_binary(long value) {
	for (int bit=7;bit>=0;bit--) {
		if (value & (1<<bit)) printf("1");
		else printf("0");
		if (bit==4 || bit==0) printf(" ");
	}
}

/* Print data at pointer as raw bits. */
void dump_binary(const void *ptr,long nbits)
{
	long nbytes=(nbits+7)/8;
	printf("dump_binary(%p, %ld bits): \n  ",ptr,nbits);
	for (long i=0;i<nbytes;i++) {
		print_binary(((const unsigned char *)ptr)[i]);
		printf(" ");
		if (i%8==7 && i!=nbytes-1) printf("\n  ");
	}
	printf("\n");
}

/* Print data at pointer as raw hex digits. */
void dump_hex(const void *ptr,long nbits)
{
	long nbytes=(nbits+7)/8;
	printf("dump_hex   (%p, %ld bits): \n  ",ptr,nbits);
	for (long i=0;i<nbytes;i++) {
		printf("%02x ",((const unsigned char *)ptr)[i]);
		if (i%8==7) printf(" ");
		if (i%32==31 && i!=nbytes-1) printf("\n  ");
	}
	printf("\n");
}

/* Print data at pointer as raw chars. */
void dump_ascii(const void *ptr,long nbits)
{
	long nbytes=(nbits+7)/8;
	printf("dump_ascii (%p, %ld bits): ",ptr,nbits);
	for (long i=0;i<nbytes;i++) {
		printf("%c",((const char *)ptr)[i]);
	}
	printf("\n");
}


/********* TraceASM support ******************/
#define machine_registers 16
struct machine_state {
	long regs[machine_registers];
	float xmm[machine_registers][4];
	long flags;
	long padding1;
};

#ifdef __cplusplus
extern "C" 
#endif
void TraceASM_cside(long line,const char *code,struct machine_state *state,long state_bytes)
{
	static const char *reg_names[machine_registers]={
		"rax","rcx","rdx","rbx",
		"rsp","rbp","rsi","rdi",
		"r8","r9","r10","r11",
		"r12","r13","r14","r15"
	};
	int i;
	static struct machine_state last;
	int nprinted=0;
	char flags[10];

	if (timer_only_dont_print) return;
	
	if (state_bytes!=sizeof(struct machine_state)) {
		printf("Error: machine state size mismatch, got %ld expected %ld",
			(long)state_bytes,(long)sizeof(struct machine_state));
		return;
	}
	
	// Decode x86 EFLAGS register
	if (state->flags & (1<< 0)) flags[0]='C'; else flags[0]='c'; // carry
	if (state->flags & (1<< 2)) flags[1]='P'; else flags[1]='p'; // parity
	if (state->flags & (1<< 6)) flags[2]='Z'; else flags[2]='z'; // zero
	if (state->flags & (1<< 7)) flags[3]='S'; else flags[3]='s'; // sign
	if (state->flags & (1<<11)) flags[4]='O'; else flags[4]='o'; // overflow
	flags[5]=0;
	
	if (line>0)
		printf("TraceASM %3ld  %-30s    %s   ",line,code,flags);
	
	// Dump any changed registers
	for (i=0;i<machine_registers;i++) {
		long v=state->regs[i];
		if (v!=last.regs[i]) {
			if (line>0) {
				// separator from previous print:
				if (nprinted>0) printf(",  ");
				
				if (i==4) // rsp changes printed relative
				{
					long diff=v-last.regs[i];
					printf("%s%s=%ld",reg_names[i],
						diff>0?"+":"-",
						diff>0?diff:-diff);
				}
				else { // ordinary register
					printf("%s=%ld (0x%lX)",reg_names[i],
						v,v);
				}
				nprinted++;
			}
			last.regs[i]=v;
		}
	}
	
	if (line>0)
		printf("\n");
}


