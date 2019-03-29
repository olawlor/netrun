/**
 Signal handler (called on program errors) used for debugging.

 Orion Sky Lawlor, olawlor@acm.org, 2005/09/14 (Public Domain)
*/
#include <stdio.h>
#include <stdlib.h>

/**
 Print a region of the stack starting at this address
  and going downward.
*/
CDECL void print_stack_range(unsigned long bp,unsigned long sp) {
	unsigned long print_start=bp+8; /* synthetic base pointer */
	int offset=0, size=(print_start-sp)/sizeof(long);
	unsigned long *sarr=(unsigned long *)sp;
	printf("Printing stack at sp=%p:\n",(void *)sp);
	if (size<0) size=20;   /* bogus bp or sp */
	if (size>20) size=20; /* big stack frame! */
	for (offset=-1;offset<=size;offset++) {
		unsigned long *p=sarr+offset;
#if defined(__amd64__)
#  define spname "rsp"
#  define bpname "rbp"
#else
#  define spname "esp"
#  define bpname "ebp"
#endif
		printf(" %p  [" spname "%+3d]",
			p,(int)(sizeof(long)*(p-sarr)));
		if (bp-sp<10000) printf(" or [" bpname "%+3d]",(int)(sizeof(long)*(p-(unsigned long *)bp)));
		printf(" =0x%016lx  (%ld)\n",
			*p,*p);
	}
}


#ifdef __i386__ 
__asm__( /* Register-saving assembly-callable stack print */
	".globl print_stack\n"
	"print_stack:\n"
	"   push %ebp;\n"
	"   mov %esp,%ebp;\n"
	"   sub $40,%esp;\n"
	" # Save all registers (for convenience)\n"
	"   mov %eax,-4(%ebp)\n"
	"   mov %ebx,-8(%ebp)\n"
	"   mov %ecx,-12(%ebp)\n"
	"   mov %edx,-16(%ebp)\n"
	"   mov %esi,-20(%ebp)\n"
	"   mov %edi,-24(%ebp)\n"
	" # Extract the old stack pointer and base pointer and print\n"
	"   lea 8(%ebp),%eax  # Original stack pointer\n"
	"   push %eax\n"
	"   push (%ebp)  # Original base pointer\n"
	"   call print_stack_range;\n"
	" # Restore registers (for convenience)\n"
	"   mov -4(%ebp),%eax\n"
	"   mov -8(%ebp),%ebx\n"
	"   mov -12(%ebp),%ecx\n"
	"   mov -16(%ebp),%edx\n"
	"   mov -20(%ebp),%esi\n"
	"   mov -24(%ebp),%edi\n"
	"   mov %ebp,%esp;  # Trim off excess stack\n"
	"   pop %ebp;\n"
	"   ret\n"
);
#endif


#if  defined(__unix__) && defined(__GNUC__) && __GNUC__>2   /* Signal handling stuff is UNIX-specific */

#include <signal.h>
#include <ucontext.h>
#include <sys/ucontext.h>
#ifdef SOLARIS /* needed with at least Solaris 8 */
#include <siginfo.h>
#endif

const static struct {
	int sigNo;
	const char *name;
} sigs[]={
	{SIGSEGV,"SIGSEGV-- segmentation violation (bad memory access)"},
	{SIGBUS,"SIGBUS-- bus error (misaligned memory access)"},
	{SIGILL,"SIGILL-- illegal instruction (jump to bad memory)"},
	{SIGFPE,"SIGFPE-- floating-point exception (divide by zero)"}
};

const char *sig2name(int sig) {
	int i,n=sizeof(sigs)/sizeof(sigs[0]); /* signal table length */
	for (i=0;i<n;i++) if (sig==sigs[i].sigNo) return sigs[i].name;
	return "Unknown signal";
}

void sig_handler(int sig, siginfo_t *HowCome, void *ucontextv) {
	static int in_signal=0;
	unsigned long pc=0,  bp=0, sp=0;
	if (in_signal++>0) exit(98); /* Don't just loop forever on signals... */
	printf("-------------------\n");
	printf("Caught signal %s (#%d)\n",sig2name(sig),sig);

#if defined(__amd64__) /* x86-64 */
	{
	ucontext_t *uc=(ucontext_t *)ucontextv;
	struct sigcontext *r=(struct sigcontext *)(&uc->uc_mcontext);
	printf("Registers:  rip=0x%16lx\n"
		"  rax=0x%16lx  rbx=0x%16lx  rcx=0x%16lx   rdx=0x%16lx\n"
		"  rsp=0x%16lx  rbp=0x%16lx  rsi=0x%16lx   rdi=0x%16lx\n"
		"  r8 =0x%16lx  r9 =0x%16lx  r10=0x%16lx   r11=0x%16lx\n"
		"  r12=0x%16lx  r13=0x%16lx  r14=0x%16lx   r15=0x%16lx\n"
		"  eflags=0x%08lx\n",
		r->rip,
		r->rax,r->rbx,r->rcx,r->rdx,
		r->rsp,r->rbp,r->rsi,r->rdi,
		r->r8,r->r9,r->r10,r->r11,
		r->r12,r->r13,r->r14,r->r15,
		r->eflags);
	pc=r->rip;
	bp=r->rbp;
	sp=r->rsp;

	}
#elif defined(__i386__) /* x86 (e.g., Pentium 4) */
	{
	ucontext_t *uc=(ucontext_t *)ucontextv;
	struct sigcontext *r=(struct sigcontext *)(&uc->uc_mcontext);
	printf("Registers:  eip=0x%08lx\n"
		"  eax=0x%08lx  ebx=0x%08lx  ecx=0x%08lx   edx=0x%08lx\n"
		"  esp=0x%08lx  ebp=0x%08lx  esi=0x%08lx   edi=0x%08lx\n"
		"  eflags=0x%08lx\n",
		r->eip,
		r->eax,r->ebx,r->ecx,r->edx,
		r->esp_at_signal,r->ebp,r->esi,r->edi,
		r->eflags);
	pc=r->eip;
	bp=r->ebp;
	sp=r->esp_at_signal;
	}
#elif defined(__PPC__) /* PowerPC (e.g., Power Macintosh) */
	{
	ucontext_t *uc=(ucontext_t *)ucontextv;
	printf("ucontext: %p\n",uc);
	struct sigcontext *mc=(struct sigcontext *)(&uc->uc_mcontext);
	printf("sigcontext: %p\n",mc);
	struct pt_regs *r=mc->regs;
	printf("regs: %p\n",r);
	int i;
	printf("Registers: nip=0x%08x   link=0x%08x   msr=0x%08x  ctr=0x%08x\n",
		r->nip, r->link,r->msr,r->ctr);
	for (i=0;i<32;i++) printf("  r%d=0x%08x",r->gpr[i]);
	printf("\n  ccr=0x%08x  dar=0x%08x  dsisr=0x%08x\n",
		r->ccr,r->dar,r->dsisr);
	pc=r->nip;
	bp=*(unsigned long *)(r->gpr[1]);
	sp=r->gpr[1];
	}
#elif defined(__arm__) 
	if (0) printf("ARM machine detected\n");
	ucontext_t *uc=(ucontext_t *)ucontextv;
	if (0) printf("ucontext: %p\n",uc);
	struct sigcontext *mc=(struct sigcontext *)(&uc->uc_mcontext);
	if (0) printf("sigcontext: %p\n",mc);
#define preg "0x%08lx "
	unsigned long *regs=&mc->arm_r0;
	printf("ARM Registers: pc=" preg " lr=" preg "  sp=" preg "\n",
		regs[15],regs[14],regs[13]);
	for (int i=0;i<16;i++)
		printf("r%d%s = " preg "%s",
			i, // register i
			i<10?" ":"", // add space after single-digit regs, to align print
			regs[i], // register value
			i%4==3?"\n":"" // break up lines
		);
	pc=regs[15];
	sp=regs[13];
	bp=sp+20;
#else
	printf("Unknown machine-- cannot dump registers...\n");
#endif
	if (bp!=0) {
		print_stack_range(bp,sp);
	}
	unsigned long fooStart=(unsigned long)(void *)&foo;
	printf("Foo routine:  0x%08lx (pc=foo+%lx)\n",fooStart,pc-fooStart);
	printf("Signal raised at address %p.  Exiting.\n", HowCome->si_addr);
	exit(99);
}

void handle_signals(void) {
	setvbuf(stdout,0,_IONBF,0); /* don't buffer stdout/stderr */
	setvbuf(stderr,0,_IONBF,0);

	int i,n=sizeof(sigs)/sizeof(sigs[0]); /* signal table length */
#if 1 // defined(__PPC__)
	stack_t stk;
	stk.ss_size=2*SIGSTKSZ;
	stk.ss_sp=((char *)malloc(2*stk.ss_size))+stk.ss_size;
	stk.ss_flags=0;
	sigaltstack(&stk,0);
#endif
	for (i=0;i<n;i++) {
        	struct sigaction sa;
        	sa.sa_sigaction = sig_handler;
        	sigemptyset( &sa.sa_mask );
        	sa.sa_flags = 0;
		sa.sa_flags |= SA_SIGINFO; /* we want a siginfo_t */
		sa.sa_flags |= SA_RESTART; /* restart interrupted syscalls 
(no EINTR errors) */
		sa.sa_flags |= SA_ONSTACK; /* use alternate signal stack 
*/
        	if (sigaction (sigs[i].sigNo, &sa, 0)) {
			perror("sigaction");
			exit(1);
        	}
	}
	if (0) fprintf(stderr,"Signal handlers loaded\n");
}

#else /* win32 or something else-- don't bother with signals */
void handle_signals(void) {
	if (0) fprintf(stderr,"Bypassing signal handlers\n");
}

#endif /* def(UNIX) signal handlers */


