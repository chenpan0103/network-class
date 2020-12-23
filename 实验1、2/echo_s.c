#define _USE_BSD

#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <string.h>

#include <stdarg.h>

#include <sys/types.h>

#include <sys/socket.h>

#include <sys/wait.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <netdb.h>

#include <signal.h>

#include <errno.h>



#define QLEN 3

#define BUFSIZE 4096

extern int errno;

unsigned short portbase = 0;



void reaper(int);

int TCPechod(int fd);

int errexit(const char *format,...);

int passivesock(const char *service, const char *transport, int qlen);

int passiveTCP(const char *service,int qlen);



int main(int argc, char *argv[]){

    char *service= "echo";

    struct sockaddr_in fsin;

    unsigned int alen;

    int msock,ssock;

    switch(argc){

case 1:

    break;

case 2:

    service=argv[1];

    break;

default:

    errexit("usage: TCPechod [port]\n");

    }



    msock=passiveTCP(service,QLEN);

    (void)signal(SIGCHLD,(__sighandler_t)QLEN);



    while(1){

        alen=sizeof(fsin);

        ssock=accept(msock,(struct sockaddr *)&fsin,&alen);

        if(ssock<0){

            if(errno==EINTR)    continue;

            errexit("accept: %s\n",strerror(errno));

        }

        switch(fork()){

    case 0:

        (void)close(msock);

        exit(TCPechod(ssock));

    default:

        (void)close(ssock);

        break;

    case -1:

        errexit("fork: %s\n",strerror(errno));

        }

    }

}



void reaper(int sig){

    int status;

    while(wait3(&status,WNOHANG,(struct rusage *)0)>=0) ;

}

int TCPechod(int fd){

    char buf[BUFSIZ];

    int cc;



    while(cc=read(fd,buf,sizeof(buf))){

        if(cc<0)

            errexit("echo read: %s\n",strerror(errno));

        if(write(fd,buf,cc)<0)

            errexit("echo write: %s\n",strerror(errno));

    }

    return 0;

}

int errexit(const char *format,...){

    va_list arg;

    va_start(arg, format);

    vfprintf(stderr,format,arg);

    va_end(arg);

    exit(1);

}

int passivesock(const char *service, const char *transport, int qlen)


{



    struct servent*pse;

    struct protoent *ppe;

    struct sockaddr_in sin;// Internet endpoint address

    int s, type;// socket descriptor and socket type



    memset(&sin, 0, sizeof(sin));

    sin.sin_family = AF_INET;

    sin.sin_addr.s_addr = INADDR_ANY;



    if ( pse = getservbyname(service, transport) )

    sin.sin_port = htons(ntohs((unsigned short)pse->s_port)+ portbase);

    else

              if ((sin.sin_port=htons((unsigned short)atoi(service)+portbase)) == 0)

    errexit("can't create passive service %d \n",sin.sin_port);



    if ( (ppe = getprotobyname(transport)) == 0)

    errexit("can't get \"%s\" protocol entry\n", transport);


    if (strcmp(transport, "udp") == 0)

    type = SOCK_DGRAM;

    else

    type = SOCK_STREAM;



   // Allocate a socket

    s = socket(PF_INET, type, ppe->p_proto);

    if (s < 0)

    errexit("can't create socket: %s\n", strerror(errno));



   // Bind the socket 

    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)

    errexit("can't bind to %s port: %s\n", service,

    strerror(errno));
   // listen

    if (type == SOCK_STREAM && listen(s, qlen) < 0)

    errexit("can't listen on %s port: %s\n", service,

    strerror(errno));

    return s;

}

int passiveTCP(const char *service,int qlen){

    return passivesock(service,"tcp",qlen);

}
