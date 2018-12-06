/* A simple TCP server--waits for a connection,
and exchanges (authenticated) messages with each
client.

See sandsend for description of what the server expects.

WARNING: Server just aborts on errors (bad data, 
security, even network timeouts), so be sure
to call this in a loop!

Orion Sky Lawlor, olawlor@acm.org, 2005/09/22 (Public Domain)
*/
#include <stdio.h>
#include <stdlib.h>
#include "auth_pipe.h"
#include "sockRoutines.h"
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string>
#include "config.h"

int main(int argc,char *argv[])
{
	skt_ip_t ip;
	unsigned int port=2983;
	int clientCount=0;
	SOCKET servFD;
	skt_init();
	if (argc>1) port=atoi(argv[1]);
	servFD=skt_server(&port);
	system("echo 'CWD: '`pwd`\"; PATH='$PATH'; ID=`id`; PID=$$\"");
	while (1)
	{
		char dest[100];
		fprintf(stdout,"SERVER> Waiting for incoming requests on port %u\n",port);
		fflush(stdout);
		SOCKET s=skt_accept(servFD,&ip,&port);
		fprintf(stdout,"SERVER> Connect from %s:%u\n", skt_print_ip(dest,ip),port);
		fflush(stdout);
		
		auth_pipe p(MY_SHARED_SECRET,auth_pipe::dir_A,s);
		
		// Receive header--version and username
		struct sand_head_t {
			Big32 version; /* Version number of request */
			char username[32]; /* nul-terminated username string */
		};
		struct sand_head_t sh;
		p.recv_start(sizeof(sh));
		p.recv(&sh,sizeof(sh));
		
		int version=sh.version;
		if ((version>>16)!=1) skt_call_abort("Incorrect major version in request!");
		
		// Reply that it's now OK to send tarfile
		p.send("OK",2);
		
		// Write name to disk
		sprintf(dest,"in_%ld_%d/",(long)time(NULL),clientCount++);
		std::string runDir=dest;
		if (0!=mkdir(runDir.c_str(),0700)) skt_call_abort("Error creating directory");
		if (0!=chdir(runDir.c_str())) skt_call_abort("Error cd'ing to directory");
		system("date > info.txt");
		system("date");
		FILE *info=fopen("info.txt","a");
		if (info==0) skt_call_abort("Error creating info file");
		fprintf(info,"User '%s', vers %x, source %s:%u\n   Tarfile contains:",
			sh.username,version,skt_print_ip(dest,ip),port);
		fclose(info);
		
		// Receive and write tarfile to disk
		int len=p.recv_start();
		fprintf(stdout,"SERVER> Receiving %d-byte file from user '%s' (vers %x) into '%s'\n",
			len,sh.username,version,runDir.c_str());
		fflush(stdout);
		const char *tarName="in.tar";
		unlink(tarName);
		FILE *tar=fopen(tarName,"wb");
		if (tar==NULL) skt_call_abort("Error creating tarfile");
		if (1!=fwrite(p.recv(len),len,1,tar)) skt_call_abort("Error writing tarfile");
		fclose(tar);
		p.recv_done();
		
		// Unpack tarfile
		mkdir("run",0777);
		if (0!=system("tar -C run -xvf in.tar | tee -a info.txt")) skt_call_abort("Error unpacking tarfile");
		// Pull out top-level directory (if one exists)
		system("cd run; [ -d * ] && mv */* .");
		
		// Run program, redirecting output to file
		int result=system("cd run; make sandrun < /dev/null > ../output 2>&1");
		
		// Send off program output (FIXME: stream this out--don't wait until end)
		FILE *out=fopen("output","rb");
		int totOutput=0;
		do {
			enum {buf_max=1024};
			unsigned char buf[buf_max];
			len=fread(buf,1,buf_max,out);
			totOutput+=len;
			p.send(buf,len);
			// printf("output send: %d bytes\n",len);
			p.send_done();
		} while (len>0);
		fclose(out);
		
		system("echo 'Program output:'; cat output");
		system("echo 'Program output:' >> info.txt; cat output >> info.txt");
		
		// Send off result code
		Big32 r(result);
		p.send(&r,sizeof(r));
		p.send_done();
		
		system("rm -fr run"); /* clean up */		
		chdir("..");

		fprintf(stdout,"SERVER> Program finished (result %d, %d bytes of output) \n",result,totOutput);
		fflush(stdout);
	}
	return 0;
}

