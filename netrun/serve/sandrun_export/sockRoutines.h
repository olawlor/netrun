/*****************************************************************************
 * $Source: /cvsroot/charm/src/util/sockRoutines.h,v $
 * $Author: olawlor $
 * $Date: 2004/12/23 22:21:36 $
 * $Revision: 1.19 $
 *****************************************************************************/

/**************************************************************************
 *
 * SKT - simple TCP and UDP socket routines.  
 *  All port numbers are taken and returned 
 *  in *host* byte order.  This means you can hardcode port
 *  numbers in the code normally, and they will be properly 
 *  translated even on little-endian machines.
 *
 *  SOCKET is just a #define for "unsigned int".
 *
 *  skt_ip_t is a flat bytes structure to hold an IP address--
 *  this is either 4 bytes (for IPv4) or 16 bytes (for IPv6).
 *  It is always in network byte order.
 *
 *  Errors are handled in the library by calling a user-overridable
 *  abort function.
 * 
 * skt_ip_t skt_my_ip(void)
 *   - return the IP address of the current machine.
 *
 * skt_ip_t skt_lookup_ip(const char *name)
 *   - return the IP address of the given machine (DNS or dotted decimal).
 *     Returns 0 on failure.
 *
 * char *skt_print_ip(char *dest,skt_ip_t addr)
 *   - Print the given IP address to the given destination as
 *     dotted decimal.  Dest must be at least 130 bytes long, 
 *     and will be returned.
 *
 * int skt_ip_match(skt_ip_t a,skt_ip_t b)
 *   - Return 1 if the given IP addresses are identical.
 *
 * SOCKET skt_datagram(unsigned int *port, int bufsize)
 *
 *   - creates a UDP datagram socket on the given port.  
 *     Performs the whole socket/bind/getsockname procedure.  
 *     Returns the actual port of the socket and
 *     the file descriptor.  Bufsize, if nonzero, controls the amount
 *     of buffer space the kernel sets aside for the socket.
 *
 * SOCKET skt_server(unsigned int *port)
 *
 *   - create a TCP server socket on the given port (0 for any port).  
 *     Performs the whole socket/bind/listen procedure.  
 *     Returns the actual port of the socket and the file descriptor.
 *
 * SOCKET skt_server_ip(unsigned int *port,skt_ip_t *ip)
 *
 *   - create a TCP server socket on the given port and IP
 *     Use 0 for any port and _skt_invalid_ip for any IP.  
 *     Performs the whole socket/bind/listen procedure.  
 *     Returns the actual port and IP address of the socket 
 *     and the file descriptor.
 *
 * SOCKET skt_accept(SOCKET src_fd,skt_ip_t *pip, unsigned int *port)
 *
 *   - accepts a TCP connection to the specified server socket.  Returns the
 *     IP of the caller, the port number of the caller, and the file
 *     descriptor to talk to the caller.
 *
 * SOCKET skt_connect(skt_ip_t ip, int port, int timeout)
 *
 *   - Opens a TCP connection to the specified server.  Returns a socket for
 *     communication.
 *
 * void skt_close(SOCKET fd)
 *   - Finishes communication on and closes the given socket.
 *
 * int skt_select1(SOCKET fd,int msec)
 *   - Call select on the given socket, returning as soon as
 *     the socket can recv or accept, or (failing that) in the given
 *     number of milliseconds.  Returns 0 on timeout; 1 on readable.
 *
 * int skt_recvN(SOCKET fd,      void *buf,int nBytes)
 * int skt_sendN(SOCKET fd,const void *buf,int nBytes)
 *   - Blocking send/recv nBytes on the given socket.
 *     Retries if possible (e.g., if interrupted), but aborts 
 *     on serious errors.  Returns zero or an abort code.
 *
 * int skt_sendV(SOCKET fd,int nBuffers,void **buffers,int *lengths)
 *   - Blocking call to write from several buffers.  This is much more
 *     performance-critical than read-from-several buffers, because 
 *     individual sends go out as separate network packets, and include
 *     a (35 ms!) timeout for subsequent short messages.  Don't use more
 *     than 8 buffers.
 * 
 * void skt_set_idle(idleFunc f)
 *   - Specify a routine to be called while waiting for the network.
 *     Replaces any previous routine.
 * 
 * void skt_set_abort(abortFunc f)
 *   - Specify a routine to be called when an unrecoverable
 *     (i.e., non-transient) socket error is encountered.
 *     The default is to log the message to stderr and call exit(1).
 *
 **************************************************************************/
#ifndef __SOCK_ROUTINES_H
#define __SOCK_ROUTINES_H

/*Preliminaries*/
#if defined(_WIN32) && ! defined(__CYGWIN__)
  /*For windows systems:*/
#include <winsock.h>
static void sleep(int secs) {Sleep(1000*secs);}

#else
  /*For non-windows (UNIX) systems:*/
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef SOCKET
#  define SOCKET int
#  define INVALID_SOCKET (SOCKET)(~0)
#  define SOCKET_ERROR (-1)
#endif /*def SOCKET*/

#endif /*WIN32*/


/*Initialization*/
void skt_init(void);

/*Error and idle handling*/
typedef void (*skt_idleFn)(void);
typedef int (*skt_abortFn)(int errCode,const char *msg);
void skt_set_idle(skt_idleFn f);
skt_abortFn skt_set_abort(skt_abortFn f);
void skt_call_abort(const char *msg);

/*DNS*/
typedef struct { /*IPv4 IP address*/
	unsigned char data[4];
} skt_ip_t;
extern skt_ip_t _skt_invalid_ip;
skt_ip_t skt_my_ip(void);
skt_ip_t skt_lookup_ip(const char *name);

char *skt_print_ip(char *dest,skt_ip_t addr);
int skt_ip_match(skt_ip_t a,skt_ip_t b);
struct sockaddr_in skt_build_addr(skt_ip_t IP,int port);

/*UDP*/
SOCKET skt_datagram(unsigned int *port, int bufsize);

/*TCP*/
SOCKET skt_server(unsigned int *port);
SOCKET skt_server_ip(unsigned int *port,skt_ip_t *ip);
SOCKET skt_accept(SOCKET src_fd, skt_ip_t *pip, unsigned int *port);
SOCKET skt_connect(skt_ip_t ip, int port, int timeout);

/*Utility*/
void skt_close(SOCKET fd);
int skt_select1(SOCKET fd,int msec);
void skt_setSockBuf(SOCKET skt, int bufsize);

/*Blocking Send/Recv*/
int skt_sendN(SOCKET hSocket,const void *pBuff,int nBytes);
int skt_recvN(SOCKET hSocket,      void *pBuff,int nBytes);
int skt_sendV(SOCKET fd,int nBuffers,const void **buffers,int *lengths);




/* Comm. utilties */

typedef unsigned char byte;

/**
Big-endian (network byte order) datatypes.

For completeness, a big-endian (network byte order) 4 byte 
integer has this format on the network:
ChMessageInt---------------------------------
  1 byte | Most significant byte  (&0xff000000; <<24)
  1 byte | More significant byte  (&0x00ff0000; <<16)
  1 byte | Less significant byte  (&0x0000ff00; <<8)
  1 byte | Least significant byte (&0x000000ff; <<0)
----------------------------------------------
*/
class Big32 { //Big-endian (network byte order) 32-bit integer
        byte d[4];
public:
        Big32() {}
        Big32(unsigned int i) { set(i); }
        operator unsigned int () const { return d[3]|(d[2]<<8)|(d[1]<<16)|(d[0]<<24); }
        unsigned int operator=(unsigned int i) {set(i);return i;}
        void set(unsigned int i) { 
                d[3]=(byte)i; 
                d[2]=(byte)(i>>8); 
                d[1]=(byte)(i>>16); 
                d[0]=(byte)(i>>24); 
        }
};


#endif /*SOCK_ROUTINES_H*/

