/* Authenticated sandbox client.

Sends to the server:
	- A username
	- An input tar file, containing at least
a Makefile with a "make sandrun" target.

Receives back from the server:
	- Program output text
	- Make result code

Orion Sky Lawlor, olawlor@acm.org, 2005/09/22 (Public Domain)
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "auth_pipe.h"
#include "config.h"

#define BLOCK 65536

void quit(const char *why) {
	fprintf(stdout,"%s\n",why?why:"");
	exit(1);
}

void usage(const char *why) {
	fprintf(stdout,
	 "Usage: sandsend [ -f <tar> ] [ -o <output> ] [ -u <username> ] <host>:<port>\n"
	 "  Send this tar file to this host and port. \n");
	quit(why);
}

/*Return the current wall clock time, in seconds*/
double walltime(void) 
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return tv.tv_sec+0.000001*tv.tv_usec;
}

int verbose=0;

const char *cur_status="Initializing";
void status(const char *str) {
	cur_status=str;
	if (verbose) fprintf(stderr,"remote> %s\n",str);
}

/*Just print out error message and exit*/
static int my_skt_abort(int code,const char *msg)
{
  fprintf(stderr,"Fatal socket error while: %s.\nError: %s (%d)\n",
  	cur_status,msg,code);
  exit(1);
  return -1;
}

int main(int argc,char *argv[])
{
	const char *tarIn=NULL;
	FILE *out=stdout;
	const char *userName="testing";
#define userNameMax 31
	skt_ip_t ip;
	unsigned int port;
	SOCKET s;
	enum {buf_max=1024};
	char buf[buf_max];
	int argi=1;

	skt_init(); skt_set_abort(my_skt_abort);
	while (argi<argc-1) {
		if (argv[argi][0]=='-')
		switch(argv[argi++][1]) {
		case 'v': verbose++; break;
		case 'f': tarIn=argv[argi++]; break;
		case 'o': {
			out=fopen(argv[argi++],"w");
			if (out==NULL) quit("Can't create output file");
		} break;
		case 'u': {
			userName=argv[argi++];
			if (strlen(userName)>=userNameMax) quit("Username too long");
		} break;
		default: usage("Invalid flag argument.");
		}
	}
	if (argi>=argc) usage("Invalid number of arguments.");
	if (2!=sscanf(argv[argi],"%[^:]:%u",buf,&port)) usage("Couldn't parse host name:port string");
	if (skt_ip_match(_skt_invalid_ip,(ip=skt_lookup_ip(buf)))) 
		  quit("Couldn't lookup host name.");

	status("Connecting");
	s=skt_connect(ip,port,10);
	status("Authenticating");
	auth_pipe p(MY_SHARED_SECRET,auth_pipe::dir_B,s);
	
	/* Send off version number and username */
	status("Sending version and username");
	struct sand_head_t {
		Big32 version; /* Version number of request */
		char username[userNameMax+1]; /* nul-terminated username string */
	};
	struct sand_head_t sh;
	sh.version=0x10000;
	strcpy(sh.username,userName);
	p.send(&sh,sizeof(sh));
	
	/* Want 2-byte "OK" string */
	enum {repl_len=2};
	p.recv_start(repl_len); 
	if (0!=strncmp((char *)p.recv(repl_len),"OK",repl_len)) quit("Didn't get OK response!\n");
	
	/* Read and send off tar file */
	status("Sending tar file");
	FILE *in=fopen(tarIn,"rb");
	int len=0;
	do {
		len=fread(buf,1,buf_max,in);
		p.send(buf,len);
	} while (len>0);
	fclose(in);
	p.send_done();
	
	/* Pull back program output */
	status("Receiving program output");
	len=0;
	do {
		len=p.recv_start();
		// printf("output recv_start: %d bytes\n",len);
		fwrite(p.recv(len),len,1,out);
	} while (len>0);
	
	/* Pull back result code & return it */
	Big32 r(0xffff);
	p.recv_start(sizeof(r));
	p.recv(&r,sizeof(r));
	
	status("Program complete");
	return r;
}

