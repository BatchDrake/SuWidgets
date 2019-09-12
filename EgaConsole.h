//
//    filename: description
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Lesser General Public License as
//    published by the Free Software Foundation, either version 3 of the
//    License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public
//    License along with this program.  If not, see
//    <http://www.gnu.org/licenses/>
//
#ifndef EGACONSOLE_H
#define EGACONSOLE_H

#include "EgaView.h"

class EgaConsole : public EgaView
{
  Q_OBJECT

  int x = 0;
  int y = 0;

public:
  EgaConsole(QWidget *parent = nullptr);

  /*
   * I've lost hours here making this work. Let this message be a reminder
   * for the future me, the importance of cleaning the build tree before
   * implenting inheritance and how useless GCC messages about vtables
   * are. The code was fine to begin with.
   */
  ~EgaConsole(void);

  void clear();
  void gotoxy(int x, int y);
  void scrollTo(int y);
  int length(void) const;
  void newLine(void);
  void setBackground(QColor const &bg);
  void setForeground(QColor const &fg);
  void print(const char *);
  void put(const char *, size_t size);

public slots:
signals:
  void scroll(void);
};

#endif // EGACONSOLE_H

