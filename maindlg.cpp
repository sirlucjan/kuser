#define _KU_MAIN
#include "maindlg.h"
#include "maindlg.moc"

#include "globals.h"
#include <qtooltip.h>
#include <kmsgbox.h>
#include <ktoolbar.h>
#include <kiconloader.h>
#include <unistd.h>
#include <signal.h>

#ifdef _KU_QUOTA
#include "quota.h"
#include "mnt.h"
#endif

#include "kuser.h"
#include "misc.h"
#include "usernamedlg.h"
#include "grpnamedlg.h"
#include "propdlg.h"
#include "addUser.h"
#include "pwddlg.h"
#include "editGroup.h"

#include <knewpanner.h>

mainDlg::mainDlg(QWidget *parent) :
QWidget(parent)
{
  changed = FALSE;
  prev = 0;

  usort = -1;
  gsort = -1;
}

void mainDlg::init() {
#ifdef _KU_QUOTA
  m = new Mounts();
  q = new Quotas();
#endif

  u = new KUsers();
  g = new KGroups();

  kp = new KNewPanner(this, "panner", KNewPanner::Horizontal);
  kp->setGeometry(10, 80, 380, 416);
  
  lbusers = new KUserView(kp, "lbusers");
  //  lbusers->setGeometry(10,80,380,208);

  lbgroups = new KGroupView(kp, "lbgroups");
  //  lbgroups->setGeometry(10,400,380,208);

  kp->activate(lbusers, lbgroups);
  
  QObject::connect(lbusers, SIGNAL(headerClicked(int)), this, SLOT(setUsersSort(int)));
  QObject::connect(lbusers, SIGNAL(selected(int)), this, SLOT(userSelected(int)));

  QObject::connect(lbgroups, SIGNAL(headerClicked(int)), this, SLOT(setGroupsSort(int)));
  QObject::connect(lbgroups, SIGNAL(selected(int)), this, SLOT(groupSelected(int)));

  reloadUsers(0);
  reloadGroups(0);
}

mainDlg::~mainDlg() {
  lbusers->setAutoUpdate(FALSE);
  lbgroups->setAutoUpdate(FALSE);

  delete lbusers;
  delete lbgroups;

  delete u;
  delete g;

#ifdef _KU_QUOTA
  delete q;
  delete m;
#endif

  delete kp;
}

void mainDlg::setUsersSort(int col) {
  if (usort == col)
    usort = -1;
  else
    usort = col;

  lbusers->sortBy(usort);

  reloadUsers(lbusers->currentItem());
}

void mainDlg::setGroupsSort(int col) {
  if (gsort == col)
    gsort = -1;
  else
    gsort = col;

  lbgroups->sortBy(gsort);

  reloadGroups(lbgroups->currentItem());
}

void mainDlg::reloadUsers(int id) {
  KUser *ku;

  lbusers->setAutoUpdate(FALSE);
  lbusers->clear();

  for (uint i = 0; i<u->getUsersNumber(); i++) {
    ku = u->getUser(i);
    lbusers->insertItem(ku);
  }

  lbusers->setAutoUpdate(TRUE);
  lbusers->setCurrentItem(id);
  lbusers->repaint();
}

void mainDlg::reloadGroups(int gid) {
  KGroup *kg;

  lbgroups->setAutoUpdate(FALSE);
  lbgroups->clear();

  for (uint i = 0; i<g->getGroupsNumber(); i++) {
    kg = g->getGroup(i);
    lbgroups->insertItem(kg);
  }

  lbgroups->setAutoUpdate(TRUE);
  lbgroups->setCurrentItem(gid);
  lbgroups->repaint();
}

void mainDlg::useredit() {
  userSelected(lbusers->currentItem());
}

void mainDlg::userdel() {
  uint i = 0;
  bool islast = FALSE;

  if (KMsgBox::yesNo(0, i18n("WARNING"),
                     i18n("Do you really want to delete user ?"),
                     KMsgBox::STOP,
                     i18n("Cancel"), i18n("Delete")) == 2) {

    i = lbusers->currentItem();
    if (i == u->getUsersNumber()-1)
      islast = TRUE;

    unsigned int uid = lbusers->getCurrentUser()->getp_uid();

#ifdef _KU_QUOTA
    if (u->user_lookup(uid) == NULL)
      q->delQuota(uid);
#endif

    char file[FILENAME_MAX], home[FILENAME_MAX];
    /*
     * Save these for later, since contents of pwd may be
     * invalidated by deletion
     */
#ifndef _PATH_MAILDIR
#define _PATH_MAILDIR "/var/mail"
#endif
    snprintf(file, sizeof(file), "%s/%s", _PATH_MAILDIR, (const char*)lbusers->getCurrentUser()->getp_name());
    strncpy(home, (const char*)lbusers->getCurrentUser()->getp_dir(), sizeof home);
    home[sizeof home - 1] = '\0';

    /*
     * Remove crontabs
     *
     * TODO: remove at jobs too.
     */
    sprintf(file, "/var/cron/tabs/%s", (const char*)lbusers->getCurrentUser()->getp_name());
    if (access(file, F_OK) == 0) {
	sprintf(file, "crontab -u %s -r", (const char*)lbusers->getCurrentUser()->getp_name());
	system(file);
    }

    u->delUser(lbusers->getCurrentUser());

    /*
     * be paranoid -- kill all processes owned by that user, if not root.
     */
    if (uid)
      switch (fork())
      {
        case 0:
          setuid(uid);
          kill(-1, 9);
          break;

        case -1:
          perror("fork");
          break;
      }

    /*
     * Remove mail file
     */
    remove(file);

    /*
     * Remove home directory and contents
     */
    if (KMsgBox::yesNo(0, i18n("WARNING"),
        i18n("The user has as homedir.\nDo you want to delete it?"),
        KMsgBox::STOP,
        i18n("Cancel"), i18n("Delete")) == 2) {
      struct stat sb;
      QString tmp;

      if(!stat(home, &sb))
        if (S_ISDIR(sb.st_mode) && sb.st_uid == uid) {
#ifdef MINIX
            tmp.sprintf("/usr/bin/rm -rf -- %s", home);
#else
            tmp.sprintf("/bin/rm -rf -- %s", home);
#endif
            system(tmp);
        }
    }

    prev = -1;

    if (!islast)
      reloadUsers(i);
    else
      reloadUsers(i-1);
    changed = TRUE;
    }
}

void mainDlg::useradd() {
  usernamedlg *ud;
  addUser *au;

  KUser *tk;
#ifdef _KU_QUOTA
  Quota *tq;
#endif // _KU_QUOTA

  int uid;

  if ((uid = u->first_free()) == -1) {
    err->display();
    return;
  }

  tk = new KUser();
  tk->setp_uid(uid);

#ifdef _KU_QUOTA
  tq = new Quota(tk->getp_uid(), FALSE);
#endif // _KU_QUOTA

  ud = new usernamedlg(tk, this);
  if (ud->exec() == 0)
    return;

  delete ud;

  config->setGroup("template");
  tk->setp_shell(readentry("shell"));
  tk->setp_dir(readentry("homeprefix")+"/"+tk->getp_name());
  tk->setp_gid(readnumentry("gid"));
  tk->setp_fname(readentry("p_fname"));
#ifdef __FreeBSD__
  tk->setp_office(readentry("p_office"));
  tk->setp_ophone(readentry("p_ophone"));
  tk->setp_hphone(readentry("p_hphone"));
  tk->setp_class(readentry("p_class"));
  tk->setp_change(readnumentry("p_change"));
  tk->setp_expire(readnumentry("p_expire"));
#else
  tk->setp_office1(readentry("p_office1"));
  tk->setp_office2(readentry("p_office2"));
  tk->setp_address(readentry("p_address"));
#endif

#ifdef _KU_SHADOW
  tk->sets_lstchg(today());
  if (config->hasKey("s_min"))
    tk->sets_min(readnumentry("s_min"));

  if (config->hasKey("s_max"))
    tk->sets_max(readnumentry("s_max"));

  if (config->hasKey("s_warn"))
    tk->sets_warn(readnumentry("s_warn"));

  if (config->hasKey("s_inact"))
    tk->sets_inact(readnumentry("s_inact"));

  if (config->hasKey("s_expire"))
    tk->sets_expire(readnumentry("s_expire"));

  if (config->hasKey("s_flag"))
    tk->sets_flag(readnumentry("s_flag"));
#endif

#ifdef _KU_QUOTA
  if (is_quota != 0)
    au = new addUser(tk, tq, this, "userin");
  else
    au = new addUser(tk, NULL, this, "userin");
#else
  au = new addUser(tk, this, "userin");
#endif

  if (au->exec() != 0) {
    u->addUser(tk);
#ifdef _KU_QUOTA
    q->addQuota(tq);
#endif
    changed = TRUE;
  }
  else {
    delete tk;
#ifdef _KU_QUOTA
    delete tq;
#endif
  }

  reloadUsers(u->getUsersNumber()-1);

  delete au;
}


void mainDlg::save() {
  if (changed == TRUE)
    if (KMsgBox::yesNo(0, i18n("Data was modified"),
                       i18n("Would you like to save changes ?"),
    		       KMsgBox::INFORMATION,
		       i18n("Save"), i18n("Discard changes")) == 1) {
      if (!u->save())
        err->display();

      if (!g->save())
        err->display();
#ifdef _KU_QUOTA
      if (is_quota != 0)
        if (!q->save())
          err->display();
#endif
    }
}

void mainDlg::quit() {
  save();

  qApp->quit();
}

void mainDlg::setpwd() {
  pwddlg *d;
  
  d = new pwddlg(lbusers->getCurrentUser(), this, "pwddlg");
  d->exec();
  changed = TRUE;
  delete d;
}

void mainDlg::help() {
  kapp->invokeHTMLHelp(0,0);
}

void mainDlg::properties() {
  propdlg *editUser;
  KUser *tk;

  tk = new KUser();
#ifdef _KU_QUOTA
  Quota *tq = new Quota(0, FALSE);
#endif

  config->setGroup("template");
  tk->setp_shell(readentry("shell"));
  tk->setp_dir(readentry("homeprefix"));
  tk->setp_gid(readnumentry("gid"));
  tk->setp_fname(readentry("p_fname"));
#ifdef __FreeBSD__
  tk->setp_office(readentry("p_office"));
  tk->setp_ophone(readentry("p_ophone"));
  tk->setp_hphone(readentry("p_hphone"));
  tk->setp_class(readentry("p_class"));
  tk->setp_change(readnumentry("p_change"));
  tk->setp_expire(readnumentry("p_expire"));
#else
  tk->setp_office1(readentry("p_office1"));
  tk->setp_office2(readentry("p_office2"));
  tk->setp_address(readentry("p_address"));
#endif

#ifdef _KU_SHADOW
  tk->sets_lstchg(readnumentry("s_lstchg"));
  tk->sets_min(readnumentry("s_min"));
  tk->sets_max(readnumentry("s_max"));
  tk->sets_warn(readnumentry("s_warn"));
  tk->sets_inact(readnumentry("s_inact"));
  tk->sets_expire(readnumentry("s_expire"));
  tk->sets_flag(readnumentry("s_flag"));
#endif

#ifdef _KU_QUOTA
  if (is_quota != 0) {
    /*
    tk->quota.at(0)->fsoft = readnumentry("quota.fsoft");
    tk->quota.at(0)->fhard = readnumentry("quota.fhard");
    tk->quota.at(0)->isoft = readnumentry("quota.isoft");
    tk->quota.at(0)->ihard = readnumentry("quota.ihard");
    */
  }
#endif

#ifdef _KU_QUOTA
  editUser = new propdlg(tk, tq, this, "userin");
#else
  editUser = new propdlg(tk, this, "userin");
#endif
  if (editUser->exec() != 0) {
    config->writeEntry("shell", tk->getp_shell());
    config->writeEntry("homeprefix", tk->getp_dir());
    config->writeEntry("gid", tk->getp_gid());
    config->writeEntry("p_fname", tk->getp_fname());
#ifdef __FreeBSD__
    config->writeEntry("p_office", tk->getp_office());
    config->writeEntry("p_ophone", tk->getp_ophone());
    config->writeEntry("p_hphone", tk->getp_hphone());
    config->writeEntry("p_class", tk->getp_class());
    config->writeEntry("p_change", tk->getp_change());
    config->writeEntry("p_expire", tk->getp_expire());
#else
    config->writeEntry("p_office1", tk->getp_office1());
    config->writeEntry("p_office2", tk->getp_office2());
    config->writeEntry("p_address", tk->getp_address());
#endif

#ifdef _KU_SHADOW
    config->writeEntry("s_lstchg", tk->gets_lstchg());
    config->writeEntry("s_min", tk->gets_min());
    config->writeEntry("s_max", tk->gets_max());
    config->writeEntry("s_warn", tk->gets_warn());
    config->writeEntry("s_inact", tk->gets_inact());
    config->writeEntry("s_expire", tk->gets_expire());
    config->writeEntry("s_flag", tk->gets_flag());
#endif

#ifdef _KU_QUOTA
    if (is_quota != 0) {
      /*
      config->writeEntry("quota.fsoft", tk->quota.at(0)->fsoft);
      config->writeEntry("quota.fhard", tk->quota.at(0)->fhard);
      config->writeEntry("quota.isoft", tk->quota.at(0)->isoft);
      config->writeEntry("quota.ihard", tk->quota.at(0)->ihard);
      */
    }
#endif
  }

  delete tk;
#ifdef _KU_QUOTA
  delete tq;
#endif
  delete editUser;
}

void mainDlg::groupSelected(int i) {
  editGroup *egdlg;
  KGroup *tmpKG;

  tmpKG = g->getGroup(i);

  if (tmpKG == NULL) {
    printf(i18n("Null pointer tmpKG in mainDlg::groupSelected(%d)\n"), i);
    return;
  }

  egdlg = new editGroup(tmpKG);

  egdlg->setCaption("Edit Groups");

  if (egdlg->exec() != 0)
    changed = TRUE;

  delete egdlg;
}

void mainDlg::userSelected(int i) {
  propdlg *editUser;
  KUser *tmpKU;

  tmpKU =  lbusers->getCurrentUser();
  if (tmpKU == NULL) {
    printf(i18n("Null pointer tmpKU in mainDlg::userSelected(%d)\n"), i);
    return;
  }

#ifdef _KU_QUOTA
  Quota *tmpQ = 0;

  if (is_quota != 0) {
    tmpQ = q->getQuota(tmpKU->getp_uid());
    if (tmpQ == NULL) {
      printf(i18n("Null pointer tmpQ in mainDlg::selected(%d)\n"), i);
      return;
    }
  }
  
  editUser = new propdlg(tmpKU, tmpQ, this, "userin");
#else
  editUser = new propdlg(tmpKU, this, "userin");
#endif

  if (editUser->exec() != 0) {
    reloadUsers(lbusers->currentItem());
    changed = TRUE;
  }

  delete editUser;
}

KUsers *mainDlg::getUsers() {
  return (u);
}

KGroups *mainDlg::getGroups() {
  return (g);
}

#ifdef _KU_QUOTA
Mounts *mainDlg::getMounts() {
  return (m);
}

Quotas *mainDlg::getQuotas() {
  return (q);
}
#endif

void mainDlg::resizeEvent (QResizeEvent *rse) {
  QSize sz;

  sz = rse->size();

  kp->setGeometry(10, 10, sz.width()-20, sz.height()-20);
//  lbusers->setGeometry(10, 10, sz.width()-20, sz.height()/2-70);
//  lbgroups->setGeometry(10, sz.height()/2+60, sz.width()-20, sz.height()/2-70);
}

void mainDlg::grpadd() {
  grpnamedlg *gd;
  int gid;
  KGroup *tk;

  if ((gid = g->first_free()) == -1) {
    err->display();
    return;
  }

  tk = new KGroup();
  tk->setgid(gid);
  
  gd = new grpnamedlg(tk, this);
  if (gd->exec() == 0) {
    delete tk;
    return;
  }

  delete gd;
  g->addGroup(tk);
  reloadGroups(lbgroups->currentItem());
  changed = TRUE;
}

void mainDlg::grpedit() {
  groupSelected(lbgroups->currentItem());
}

void mainDlg::grpdel() {
  uint i = 0;
  bool islast = FALSE;

  if (KMsgBox::yesNo(0, i18n("WARNING"),
                     i18n("Do you really want to delete group ?"),
                     KMsgBox::STOP,
                     i18n("Cancel"), i18n("Delete")) == 2) {

    i = lbgroups->currentItem();
    if (i == g->getGroupsNumber()-1)
      islast = TRUE;

    g->delGroup(lbgroups->getCurrentGroup());

    prev = -1;

    if (!islast)
      reloadGroups(i);
    else
      reloadGroups(i-1);
    changed = TRUE;
  }
}

