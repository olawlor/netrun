/* A simple TCP authenticated messaging protocol.

Orion Sky Lawlor, olawlor@acm.org, 2005/09/22 (Public Domain)
*/
#include <stdio.h>
#include <stdlib.h>
#include "sockRoutines.h"
#include "osl/sha1.h"
#include "auth_pipe.h"

using osl::SHA1_hash_t;

static FILE *good_rand_src=0;
int good_rand(void) {
	if (good_rand_src==0) {
		good_rand_src=fopen("/dev/urandom","rb");
		if (good_rand_src==0) return rand();
	}
	int i;
	fread(&i,1,sizeof(i),good_rand_src);
	return i;
}


auth_pipe::auth_pipe(const char *sharedSecret_,dir_t d,SOCKET fd_)
	:fd(fd_),sharedSecret(sharedSecret_),state(state_idle)
{
	int n;
	Big32 my[n_nonce], his[n_nonce];
	for (n=0;n<n_nonce;n++) my[n]=good_rand();
	skt_sendN(fd,&my,sizeof(my));
	skt_recvN(fd,&his,sizeof(his));
	for (n=0;n<n_nonce;n++) {
		nonce[( d)*n_nonce+n]=my[n];
		nonce[(!d)*n_nonce+n]=his[n];
	}
	count=0;
}
void auth_pipe::reset(void) {
	h.end(); /* Clean out any invalid data */
	h.addBytes(&nonce,sizeof(nonce)); /* Add nonces to hash */
	count=count+1;
	h.addBytes(&count,sizeof(count)); /* Add count to hash */
	h.addBytes(sharedSecret,strlen(sharedSecret)); /* Add secret to hash */
	msgidx=0;
}

void auth_pipe::send_start(void) {
	if (state==state_recv) recv_done();
	reset();
	state=state_send;
}

void auth_pipe::send(const void *b,int len)
{
	if (state!=state_send) send_start();
	msg.resize(msgidx+len);
	memcpy(&msg[msgidx],b,len);
	h.addBytes(b,len);
	msgidx+=len;
}
void auth_pipe::send_done(void)
{
	if (state!=state_send) skt_call_abort("Logic error: send_done called outside send mode!");

	// Send message length
	Big32 msglen=msgidx;
	skt_sendN(fd,&msglen,sizeof(msglen));
	
	// Append hashcode to message data & send
	SHA1_hash_t hc=h.end();
	send((const byte *)&hc,sizeof(hc)); 
	skt_sendN(fd,&msg[0],msglen+sizeof(hc));

	msg.resize(0);
	state=state_idle;
}

int auth_pipe::recv_start(int expect)
{
	flush();
	reset();
	state=state_recv;
	
	// Grab message length
	Big32 msglen;
	skt_recvN(fd,&msglen,sizeof(msglen));
	if ((expect!=-1) && ((int)msglen!=expect)) skt_call_abort("Security error: message length mismatch!");
	if (msglen<0 || msglen>=10*1024*1024) skt_call_abort("Security error: message length bogus!");
	
	// Grab message data + hash code
	msgidx=msglen+sizeof(SHA1_hash_t);
	msg.resize(msgidx);
	skt_recvN(fd,&msg[0],msgidx);
	h.addBytes(&msg[0],msglen);
	SHA1_hash_t hc=h.end();
	if (SHA1_differ(&hc,(const SHA1_hash_t *)&msg[msglen])) skt_call_abort("Security error: message auth mismatch!\n");
	msg.resize(msglen); /* Clip off hash code, so it doesn't get returned */
	msgidx=0;
	return msglen;
}

const byte *auth_pipe::recv(int len)
{
	if (state!=state_recv) recv_start();
	
	if ((unsigned int)(len+msgidx)>msg.size()) skt_call_abort("Logic (or security) error: trying to receive too many bytes!\n");
	const byte *ret=&msg[msgidx];
	msgidx+=len;
	return ret;
}

void auth_pipe::recv(void *dest,int len)
{
	memcpy(dest,recv(len),len);
}

/* Return bytes left to receive */
int auth_pipe::recv_left(void) {
	if (state!=state_recv) return 0;
	return msg.size()-msgidx;
}

void auth_pipe::recv_done(void) {
	if (state!=state_recv) skt_call_abort("Logic error: recv_done called outside recv mode!");
	msg.resize(0);
	state=state_idle;
}

void auth_pipe::flush(void) {
	if (state==state_send) send_done();
	if (state==state_recv) recv_done();
}

auth_pipe::~auth_pipe() {
	flush();
	skt_close(fd);
}
