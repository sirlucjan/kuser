#ifdef _KU_QUOTA
#include <stdlib.h>
#include <stdio.h>

#include "mnt.h"
#include "kuqconf.h"

#include <qfile.h>

MntEnt::MntEnt(const QString &afsname, const QString &adir,
               const QString &atype, const QString &aopts,
               const QString &aquotafilename){
    fsname = afsname;
    dir    = adir;
    type   = atype;
    opts   = aopts;
    quotafilename = aquotafilename;
}

MntEnt::~MntEnt() {
}

QString MntEnt::getfsname() const {
  return fsname;
}

QString MntEnt::getdir() const {
  return dir;
}

QString MntEnt::gettype() const {
  return type;
}

QString MntEnt::getopts() const {
  return opts;
}

QString MntEnt::getquotafilename() const {
  return quotafilename;
}

void MntEnt::setfsname(const QString &data) {
  fsname = data;
}

void MntEnt::setdir(const QString &data) {
  dir = data;
}

void MntEnt::settype(const QString &data) {
  type = data;
}

void MntEnt::setopts(const QString &data) {
  opts = data;
}

void MntEnt::setquotafilename(const QString &data) {
  quotafilename = data;
}

Mounts::Mounts() {
  Mounts::m.setAutoDelete(TRUE);

#ifdef OLD_GETMNTENT
  struct mnttab *mt = NULL;
#elif defined(BSD)
  struct fstab *mt = NULL;
#elif defined(_AIX)
  struct fstab *mt = NULL;
  struct vfs_ent *vt = NULL;
#else
  struct mntent *mt = NULL;
#endif
#if !defined(BSD) &&!defined(_AIX)
  FILE *fp;
#endif
  MntEnt *mnt = NULL;
  QString quotafilename;

  if (is_quota == 0)
    return;
  is_quota = 0;

#ifdef OLD_GETMNTENT
  fp = fopen(MNTTAB, "r");
  mt = (struct mnttab *)malloc(sizeof(mnttab));

  while ((getmntent(fp, mt)) == 0) {
    if (strstr(mt->mnt_mntopts, "quota") == NULL)
      continue;

    if (!CORRECT_FSTYPE((const char *)mt->mnt_fstype))
      continue;

    quotafilename = QString("%1%2%3")
                          .arg(mt->mnt_mountp)
                          .arg((mt->mnt_mountp[strlen(mt->mnt_mountp) - 1] == '/') ? "" : "/")
                          .arg(_KU_QUOTAFILENAME);
#elif defined(BSD) /* Heh, heh, so much for standards, eh Bezerkely? */
  while ((mt=getfsent()) != NULL) {
    if (strstr(mt->fs_mntops,"quota")==NULL)
      continue;
    if (strcasecmp(mt->fs_vfstype,"ufs") != 0)
      continue;
    quotafilename = QString("%1%2%3")
                          .arg(mt->fs_file)
                          .arg((mt->fs_file[strlen(mt->fs_file) -1] == '/') ? "" : "/")
 		                      .arg(_KU_QUOTAFILENAME);
#elif defined(_AIX)
  while ((vt=getvfsent()) != NULL) {
    /* The prototype of getfstype() is botched (old K&R without args).
     * Or alternatively the man page is, in which case this code
     * is trash. */
    while ((mt=(*(getfstype_proto)&getfstype)(vt->vfsent_name)) != NULL) {
      if (strstr(mt->fs_spec,FSTAB_RQ)==NULL)
        continue;
      // if (strcasecmp(mt->fs_vfstype,"ufs") != 0) continue;
      quotafilename = QString("%1%2%3")
                            .arg(mt->fs_file)
                            .arg((mt->fs_file[strlen(mt->fs_file) -1] == '/') ? "" : "/")
                            .arg(_KU_QUOTAFILENAME);
#else
  fp = setmntent(MNTTAB, "r");
  while ((mt = getmntent(fp)) != (struct mntent *)0) {
    if (strstr(mt->mnt_opts, "quota") == NULL)
      continue;

    if (!CORRECT_FSTYPE((const char *)mt->mnt_type))
      continue;

    quotafilename = QString("%1%2%3")
                          .arg(mt->mnt_dir)
                          .arg((mt->mnt_dir[strlen(mt->mnt_dir) - 1] == '/') ? "" : "/")
                          .arg(_KU_QUOTAFILENAME);

#endif

    QFile *f = new QFile(quotafilename);
    if (f->exists() == FALSE) {
      printf("Quota file name %s does not exist\n", quotafilename.local8Bit().data());
      continue;
    }
    printf("Quota file name %s found\n", quotafilename.local8Bit().data());
#ifdef OLD_GETMNTENT
    mnt = new MntEnt(mt->mnt_special, mt->mnt_mountp, mt->mnt_fstype,
                     mt->mnt_mntopts, quotafilename);
    m.append(mnt);
    is_quota = 1;
  }
  fclose(fp);
#elif defined(BSD)
    mnt = new MntEnt(mt->fs_spec,mt->fs_file,mt->fs_vfstype,
		   mt->fs_mntops,quotafilename);
    m.append(mnt);
    is_quota = 1;
  }
  endfsent();
#elif defined(_AIX)
      /* Are the mount options parsed away by the mount helpers?
       * I can't find them in the structures.
       */
      mnt = new MntEnt(mt->fs_spec,mt->fs_file,vt->vfsent_name,
		     "",quotafilename);
      m.append(mnt);
      is_quota = 1;
    }
    endfsent();
  }
  endvfsent();
#else
    mnt = new MntEnt(mt->mnt_fsname, mt->mnt_dir, mt->mnt_type,
                     mt->mnt_opts, quotafilename);
    m.append(mnt);
    is_quota = 1;
  }
  endmntent(fp);
#endif
}

Mounts::~Mounts() {
  m.clear();
}

MntEnt *Mounts::operator[](uint num) {
  return (m.at(num));
}

uint Mounts::getMountsNumber() {
  return (m.count());
}
#endif
