/* 
Run a program inside an "chroot" jail as an unprivileged user:
	- Make a "run" directory
	- Hardlink program and needed libraries into run directory
	- Fork off a child, drop privileges, and exec program
	- Kill program if it runs too long

The permissions in the run directory should not allow setuid
executables, direct hardware devices, etc.

This program must be setuid root, or called as root,
as required by chroot.

Orion Sky Lawlor, olawlor@acm.org 2006/09/22 (Public Domain)
*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h> /* for signal */
#include <unistd.h> /* for link, chroot, etc. */
#include <sys/stat.h> /* for mkdir */
#include <sys/time.h>
#include <sys/wait.h>
#ifdef SOLARIS /* needed with at least Solaris 8 */
#include <siginfo.h>
#endif

/* Configuration defines.  Override these from the Makefile */
#ifndef runUser
#  define runUser 6661313 /* *unprivileged* user ID to run as */
#endif
#ifndef runTime
#  define runTime 2 /* seconds to allow program to run before killing it */
#endif
#define runDir "run" /* new directory to run code inside */
#define exeName "code.exe" /* executable's new name in run directory */

/* This directory contains all the shared libraries your program needs.
 You can initially create libSrc by copying everything listed in `ldd executable`
*/
#ifndef libSrc
#  define libSrc "/usr/local/bin/s4g_libs/" /* library files to hardlink */
#endif
#if __NetBSD__
#  define libDir "usr/lib"
#endif
#ifndef libDir
#  define libDir "lib" /* library directory name in the run directory */
#endif

int childPID=0;
/** Call wait() to allow the child process to finish.
  If termFirst is true, make the child finish immediately.
  Otherwise just wait nicely for the child.
  WARNING: This routine runs as root! 
*/
void waitForChild(int termFirst) {
	if (childPID==0) exit(0); /* child not yet created! */
	//printf("Killing process %d\n",childPID);
	if (termFirst) goto terminate;
	while (kill(childPID,0)==0) 
	{ /* child process is still there-- wait on it */
		int status=0;
		wait(&status);
	terminate: 
		/* try killing the child's entire process group */
		kill(-childPID,SIGKILL);
		/* And also try killing just the child */
		kill(childPID,SIGKILL);
	}
	if (1) {
	/* As a backup, kill everything in the nobody account.
	   This isn't foolproof, and is probably not needed. */
		setuid(runUser);
		kill(-1,SIGKILL);
	}
	exit(0);
}

void signalHandler(int cause) {
	printf("Killing program--ran too long!\n");
	waitForChild(1);
}
void bad(int err,const char *fn) {
	perror("Error");
	printf("Error %d returned during execution of syscall '%s'\n",err,fn);
	waitForChild(1);
}

#define check(fn,args) \
	{ int err=fn args; if (err!=0) {bad(err,#fn);}}
#define nocheck(fn,args) fn args

#include <sys/resource.h> /* FIXME: is this linux-specific? */
void my_limit(int resource,int val) {
	struct rlimit r; 
	r.rlim_cur=val;
	r.rlim_max=val;
	setrlimit(resource,&r);
}


int main(int argc,char *argv[]){ 
	struct itimerval itimer;
	
/* Paranoia */
	seteuid(getuid()); /* give up setuid privileges before doing anything else http://yarchive.net/comp/setuid_mess.html */
	unsetenv("IFS"); /* used by shell */
	unsetenv("PATH");
	unsetenv("LD_LIBRARY_PATH");
	/*clearenv();*/

/* Parse command line */
	if (argc<=1) {
		printf("Usage: s4g_chroot <exe> <args>\n"
			"   Runs this executable in a little chroot jail built in the '" runDir "' directory.\n");
		return 1;
	}
	
/* Create rundir */
	//check(system,("/bin/rm -fr "runDir)); /* (leftover stuff could be sensitive or dangerous) */
	nocheck(mkdir,(runDir,0777)); /* 777 since program needs to be able to create files... */
	nocheck(chmod,(runDir,0777)); /* override umask */
	nocheck(link,(argv[1],runDir "/" exeName)); /* hardlink over executable */
	check(chdir,(runDir)); /* everything else happens inside the run directory */

#if defined(__linux__) && defined(__LP64__)
/* Linux 64-bit executables need /lib64/ld-linux-x86-64.so.2 */
	nocheck(mkdir,("lib64",0777));
#define ldlinux "ld-linux-x86-64.so.2"
	nocheck(link,(libSrc "/" ldlinux,"lib64/"ldlinux));
#endif

#if __NetBSD__  
        nocheck(system,("mkdir usr"));
	nocheck(system,("mkdir usr/libexec"));
	nocheck(system,("cp /usr/libexec/ld.elf_so usr/libexec"));
#endif  

/* Make soft and hardlinks for executable and dynamic libraries */
	nocheck(mkdir,(libDir,0755)); /* fill up lib directory with needed libs */
	nocheck(system,("/bin/ln -f '" libSrc "'/* " libDir));

	signal(SIGALRM,signalHandler);

/* Request a SIGALRM after runTime seconds */
	itimer.it_interval.tv_sec=0;
	itimer.it_interval.tv_usec=0;
	itimer.it_value.tv_sec=runTime;
	itimer.it_value.tv_usec=0;
	check(setitimer,(ITIMER_REAL, &itimer, NULL)); 
	
/* su and chroot.  WARNING: REMAINING PARENT CODE RUNS AS ROOT! 
  Rationale: Parent can't be same nobody user as the child code, 
  since then the child code could kill off its controlling parent.
  But parent then has to be root to kill child, since only root can
  kill a process you don't own.
*/
	check(setuid,(0));
	check(chroot,("."));
	
/* Run program */
	childPID=fork();
	if (childPID==0) { /* we're the child! */
		/* Stuff child into separate process group; see
			http://www.win.tue.nl/~aeb/linux/lk/lk-10.html 
		Subtle: calling setpgid(childPID,0) from the parent is 
		subject to a race condition--child might fork before we setpgid,
		leaving some fork'ed grandchildren outside the process group...
		*/
		setpgid(0,0); 
		
		/* Child process always runs as nobody user */
		check(setuid,(runUser));
	
	/* Decrease resource limits, so child can't make much trouble... */
		nice(5); /* don't hammer CPU */
		my_limit(RLIMIT_CORE,0); /* size of core file */
		my_limit(RLIMIT_CPU,runTime+1); /* seconds of CPU (backup, in case parent fails) */
		my_limit(RLIMIT_DATA,100*1024*1024); /* bytes of brk()  */
		my_limit(RLIMIT_RSS,100*1024*1024); /* bytes of resident RAM */
		my_limit(RLIMIT_FSIZE,1*1024*1024); /* bytes of created files size */
		my_limit(RLIMIT_MEMLOCK,1*1024*1024); /* bytes of locked memory */
		my_limit(RLIMIT_NOFILE,100); /* number of open files */
		my_limit(RLIMIT_NPROC,9); /* number of fork'd processes/threads (0==disable fork entirely)  */
		/* FIXME: remaining vulnerabilities: outgoing network traffic */
	
		execv(exeName,&argv[1]);
		perror("execv failure");
		printf("Sadly, execv('%s') failed.  This is usually a shared library problem--check 'ldd %s/%s', and make sure all listed libraries are in the '%s' directory (and copied into '%s/%s').\n", exeName, runDir,exeName, libSrc, runDir,libDir);

	} else { /* parent--wait for child */
		waitForChild(0);
	}
        return(0);
}

