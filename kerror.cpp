/*
 *  Copyright (c) 1998 Denis Perchine <dyp@perchine.com>
 *  Maintained by Adriaan de Groot <groot@kde.org>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 **/

#include "misc.h"
#include "kerror.h"
#include <kmessagebox.h>

KError::KError() {
}

KError::~KError() {
}

void KError::addMsg(QString amsg) {
  msgs.append(amsg);
}

void KError::display() {
  for (uint i=0;i<msgs.count();i++)
     KMessageBox::information(0, msgs[i]);
  msgs.clear();
}

