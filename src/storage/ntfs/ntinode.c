//
// Created by Kyle Cesar on 07/08/21.
//

#include "../../libs/common.h"
#include "../../libs/sync.h"
#include "../../libs/scb.h"
#include "../../libs/stat.h"
#include "../../libs/region.h"

#include "../../libs/inode.h"
#include "../../libs/bhead.h"
#include "../../mntent.h"
#include "../../libs/sb.h"
#include "../../libs/nt.h"
#include "../../libs/sysdirent.h"
#include "../../libs/uerror.h"
#include "../../libs/signal.h"
#include "../../libs/uproc.h"

#include "../../libs/extern.h"
#include "../../libs/proto.h"


time_t  ntfs_nttimetounix (long long nt);
void ntfs_free_ntvatter (struct ntvattr *vap);

int nt_read_inode (INODE *ip) {
    const SB    *sp = ip->i_sb;
    NTSB        *ntsp = sp->sb_ptr;
    int isdir;
    struct filerec       *mfrp;
    struct attr          *ap;
    struct ntvattr       *vap, *last_vap;
    const struct ntvattr *data_vap = NULL,*name_vap = NULL;
    IOREQ io;

#ifdef DEBUG
    printf ("nt_read_inode: Read inode attributes %d\n", ip->i_ino);
#endif DEBUG

    mfrp = malloc_byte (ntsp->s_MFT_record_size);

    io.io_dev   = ip->i_dev;
    io.io_ip    = ntsp->s_mft_inode;
    io.io_area  = mfrp;
    io.io_count = ntsp->s_MFT_record_size;
    io.io_cpflag = SS;

    if (ip->i_ino < NTFS_SYSNODESNUM) {
        ino_t       ino;

        if ((ino = ip->i_ino) == NTFS_PSEUDO_MFTINO)
            ino = NTFS_MFTINO;

        io.io_offset = ((long long)ntsp->s_MFT_cluster_no << ntsp->s_CLUSTER_SHIFT)
                + ino * ntsp->s_MFT_record_size;

        if (ntfs_bread (&io) < 0)
            goto bad;
    } else {
        io.io_offset_low = ip->i_ino * ntsp->s_MFT_record_size;

        if (ntfs_readattr(ntsp, NTFS_A_DATA, NULL, &io) < 0)
            goto bad;
    }

    if (ntfs_procfixups(ntsp,NTFS_FILEMAGIC, mfrp, ntsp->s_MFT_record_size) < 0)
        goto bad;

    ip->i_attr_list = NULL; last_vap = NULL;

    for (
            ap = (struct attr *)((char *)mfrp + mfrp->fr_attroff);
            ap->a_hdr.a_type != -1;
            ap = (struct attr *)((char *)ap + ap->a_hdr.a_reclen)
            )
    {
        if ((vap = ntfs_attrtontvattr(ntsp, ap)) == NULL)
            goto bad;

        switch (vap->va_type) {
            case NTFS_A_ATTRLIST:
            case NTFS_A_VOLUMENAME:
            case NTFS_A_INDXROOT:
            case NTFS_A_INDX:
            case NTFS_A_INDXBITMAP:
                break;

            case NTFS_A_NAME:
                name_vap = vap;
                break;

            case NTFS_A_DATA:
                data_vap = vap;
                break;

            default:

#ifdef DEBUG
    printf ("nt_read_inode: Ignore attribute %s (%d bytes), ino = %d\n",
    ntfs_edit_attrib_type (vap->va_type), ntfs_get_ntvatter_size (vap), ip->i_ino);
#endif DEBUG

        ntfs_free_ntvatter(vap);
        continue;
        }

#ifdef DEBUG
    printf("nt_read_inode: Add atributte %s, ino = %d\n",
           ntfs_edit_attrib_type (vap->va_type), ip->i_ino);
#endif DEBUG

        if (last_vap == NULL)
            ip->i_attr_list = (int)vap;
        else
            last_vap->va_next = vap;

        last_vap = vap; vap->va_next = NULL;

    }

    isdir = (mfrp->fr_flags & 2);

    if (isdir)
        ip->i_mode = S_IFDIR | S_IEXEC | (S_IEXEC >> IPSHIFT) | (s_IEXEC >> (2 * IPSHIFT));
    else
        ip->i_mode = S_IFREG | S_IEXEC;

    ip->i_mode |= S_IREAD | (S_IREAD >> IPSHIFT) | (S_IREAD >> (2 * IPSHIFT));

    ip->i_uid = sp->sb_uid;
    ip->i_uid = sp->sb_gid;

#if (0)
    if ((ip->i_nlink = mfrp->fr_nlink) <= 0)
        ip->i_nlink =1;
#endif

    if (name_vap != NULL) {
        ip->i_atime = ntfs_nttimetounix(name_vap->va_a_name->n_times.t_access);
        ip->i_mtime = ntfs_nttimetounix(name_vap->va_a_name->n_times.t_write);
        ip->i_ctime = ntfs_nttimetounix(name_vap->va_a_name->n_times.t_create);

    }

    if (isdir) {
        ip->i_last_entryno   = 0;
        ip->i_last_type      = 0;
        ip->i_last_offset    = 0;
        ip->i_last_dir_blkno = 0;
        ip->i_total_entrynos = 0;
    }

    ip->i_fsp   = &fstab[FS_NT];
    ip->i_rdev  = 0;

    free_byte (mfrp);

    ip->i_size = 0;

    if (!isdir) {
        if (data_vap == NULL) {
            if ((data_vap = ntfs_ntvattrget(ntsp,ip,NTFS_A_DATA, NOSTR,0,0))) {
                printf("%g: No found datas of inode %d\n", ip->i_ino);
                return (0);
            }
        }
        ip->i_size = data_vap->va_datalen;
        ip->i_nlink = 1;
    } else {
        nt_get_nlink_dir(ip);
    }
    return (0);

    bad:
    printf("%g: Error on read of Inode (%v, %d)\n", ip->i_dev,ip->i_ino);

    free_byte(mfrp);

    return (-1);
}

void nt_write_inode (INODE *ip) {
    if ((ip->i_flags & ICHG) == 0)
        return;

    inode_dirty_dec(ip);

    if (ip->i_sb->sb_mntent.me_flags & SB_RONLY)
        return;
}

void nt_trunc_inode (INODE *ip) {

}

void nt_free_inode (const INODE *ip) {

}

time_t ntfs_nttimetounix (long long nt) {
    time_t      t;

    t = nt / (1000 * 1000 * 10) -
            369LL * 365LL * 24LL * 60LL * 60LL -
            89LL * 1LL * 24LL * 60LL * 60LL;

    return (t);
}

void ntfs_free_all_ntvattr (INODE *ip) {
    struct ntvattr *var, *next_vap;

#ifdef DEBUG

#endif DEBUG

    if(ip->i_sb->sb_code != FS_NT) {
        printf("%g: INODE (%v,%d) of devices is dont NTFS filesystem\n",ip->i_sb->sb_dev,ip->i_ino);
        return;
    }
    for (vap = (struct ntvattr *)ip->i_attr_list;vap != NULL;vap = next_vap)
    {next_vap = vap->va_next;
        ntfs_free_ntvatter(var);}

    ip->i_attr_list = NULL;
}

void ntfs_free_ntvattr (struct ntvattr *vap) {
    if (vap->va_flag & NTFS_AF_INRUN) {
        free_byte(vap->va_vruncn);
        free_byte(vap->va_vruncl);
    } else {
        free_byte(vap->va_datap);
    }
    free_byte(vap);
}