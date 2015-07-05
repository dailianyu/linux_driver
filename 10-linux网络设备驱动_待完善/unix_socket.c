/*
	longer, longer.zhou@gmail.com
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <stdio.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h> /* for exit */

extern FILE *logfp;


int
init_unix_socket(const char *local_addr)
{
	struct  sockaddr_un clntaddr;/* address of client */
	int sock;

	unlink(local_addr);
	
	 /* Create a UNIX datagram socket for client */
	if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        fprintf(logfp,"client: socket %s\n",strerror(errno));
        exit(2);
    }	

	/*      Client will bind to an address so the server will
	*       get an address in its recvfrom call and use it to
	*       send data back to the client.
	*/
    //bzero(&clntaddr, sizeof(clntaddr));
    memset(&clntaddr, 0 ,sizeof(clntaddr));
    clntaddr.sun_family = AF_UNIX;
    strcpy(clntaddr.sun_path, local_addr);
   
    if (bind(sock, &clntaddr, sizeof(clntaddr)) < 0) {
		close(sock);
		fprintf(logfp,"client: bind %s\n",strerror(errno));
		exit(3);
    }

	return sock;
	
}

int 
Recvfrom(	int sockfd, void *buff, 
			size_t nbytes, int flags, 
			void **from_addr, int *from_len)
{
	int slen, fromlen;
	struct sockaddr_un *from;

	fromlen = sizeof(*from);
	from = malloc(fromlen);
	

	slen = recvfrom(sockfd,buff,nbytes,flags,from, &fromlen);

	if (from_addr && from_len)	{
		*from_addr  = from;
		*from_len  = fromlen;
	}
	return slen;
}

int
Recv(int sockfd, void *buff, size_t nbytes, int flags)
{
	int slen;

	slen = recv(sockfd, buff, nbytes, flags);

	return slen;
}

int Sendto(int sockfd, const void *buff, 
				size_t nbytes,int flags, 
				const char *to_addr)
{
	int slen ;
	struct sockaddr_un  to;

	 /*   Set up address structure for destination  socket  */
    memset(&to, 0, sizeof(to));
    to.sun_family = AF_UNIX;
    strcpy(to.sun_path, to_addr);
	
    slen = sendto(sockfd, buff, nbytes, flags,
          (struct sockaddr *) &to,
            sizeof(to));
	return slen;
}


