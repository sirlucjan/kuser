#ifndef _KU_QUOTATOOL_H_
#define _KU_QUOTATOOL_H_

#ifdef _KU_QUOTA

#include <qlist.h>
#include <qintdict.h>

#include "globals.h"

#include <sys/file.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#ifdef HAVE_MNTENT_H
#include <mntent.h>
#endif

#ifdef HAVE_SYS_MNTENT_H
#include <sys/mntent.h>
#define OLD_GETMNTENT
#endif

#ifdef HAVE_SYS_MNTTAB_H
#include <sys/mnttab.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

#ifdef HAVE_SYS_FS_UFS_QUOTA_H
#include <sys/fs/ufs_quota.h>
#define CORRECT_FSTYPE(type) (!strcmp(type,MNTTYPE_UFS))
#define _KU_QUOTAFILENAME "quotas"
#define _KU_UFS_QUOTA
#else
#ifdef HAVE_LINUX_QUOTA_H
#if defined __GLIBC__ && __GLIBC__ >= 2
typedef unsigned int __u32;
#define MNTTYPE_EXT2 "ext2"
#endif
#include <linux/quota.h>
#define CORRECT_FSTYPE(type) (!strcmp(type,MNTTYPE_EXT2))
#define _KU_QUOTAFILENAME "quota.user"
#define _KU_EXT2_QUOTA
#else
#ifdef HAVE_IRIX
#include <sys/quota.h>
#define CORRECT_FSTYPE(type) (!strcmp(type,MNTTYPE_EFS) || !strcmp(type,MNTTYPE_XFS))
#define _KU_QUOTAFILENAME "quotas"
#else
#error "Your platform is not supported"
#endif // HAVE_IRIX
#endif // HAVE_SYS_FS_UFS_QUOTA_H
#endif // HAVE_LINUX_QUOTA_H

class QuotaMnt {
public:
  long int
    fcur,
    fsoft,
    fhard,
    icur,
    isoft,
    ihard,
    ftime,
    itime;
  QuotaMnt();
  QuotaMnt(int afcur, int afsoft, int afhard, int aicur, int aisoft, int aihard);
  QuotaMnt(int afcur, int afsoft, int afhard, int aicur, int aisoft, int aihard, int aftime, int aitime);
  QuotaMnt(const QuotaMnt *q);
  ~QuotaMnt();
  QuotaMnt& operator= (const QuotaMnt& q);
};

class Quota {
public:
  Quota(long auid);
  ~Quota();

  QuotaMnt *getQuotaMnt(uint mntnum);
  uint getMountsNumber();
  void save();
protected:
  long uid;
  QList<QuotaMnt> q;
};

class Quotas {
public:
  Quotas();
  ~Quotas();

  Quota *getQuota(long uid);
  void addQuota(long uid);
  void save();
 protected:
  QIntDict<Quota> q;
};

#endif // _KU_QUOTA

#endif // _KU_EDQUOTA_H_
