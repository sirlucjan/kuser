#ifndef _KERROR_H
#define _KERROR_H

#include <qobject.h>
#include <qlist.h>
#include <qstring.h>

class KErrorMsg {
public:
  enum KErrorType {
    INFORMATION = 1,
    EXCLAMATION = 2,
    STOP = 4,
    QUESTION = 8
  };

  KErrorMsg(const char *amsg, KErrorType aerr);
  ~KErrorMsg();
  
  const char *getMsg();
  KErrorType getErr();
  
private:
  QString msg;
  KErrorType err;
};

class KError: public QObject {
public:

  KError();
  ~KError();
  
  void addMsg(const char *amsg, KErrorMsg::KErrorType aerr);
  void display();
 
private:
  QList<KErrorMsg> msgs;
};

#endif
// _KERROR_H
