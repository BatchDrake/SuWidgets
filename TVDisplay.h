//
//    TVDisplay.h: Television Display Widget
//    Copyright (C) 2020 Gonzalo José Carracedo Carballal
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
#ifndef TVDISPLAY_H
#define TVDISPLAY_H

#include <QFrame>

#include <cmath>
#include <complex.h>
#include <vector>
#include <tgmath.h>
#include <sigutils/types.h>
#include "ThrottleableWidget.h"

#define TVDISPLAY_DEFAULT_BACKGROUND_COLOR QColor(0,     0,   0)
#define TVDISPLAY_DEFAULT_FOREGROUND_COLOR QColor(255, 255, 255)

class TVDisplay : public ThrottleableWidget
{
  Q_OBJECT

  Q_PROPERTY(
      QColor backgroundColor
      READ getBackgroundColor
      WRITE setBackgroundColor
      NOTIFY backgroundColorChanged)

  Q_PROPERTY(
      QColor foregroundColor
      READ getForegroundColor
      WRITE setForegroundColor
      NOTIFY foregroundColorChanged)


  // Drawing area properties
  QPixmap contentPixmap;

  QSize geometry;

  // Data
  unsigned int x = 0;
  unsigned int y = 0;
  bool oddFrame = false;

  // Properties
  QColor background;
  QColor foreground;

  void drawAxes(void);
  void drawTVDisplay(void);

public:
  void
  setBackgroundColor(const QColor &c)
  {
    this->background = c;
    this->invalidate();
    emit backgroundColorChanged();
  }

  const QColor &
  getBackgroundColor(void) const
  {
    return this->background;
  }

  void
  setForegroundColor(const QColor &c)
  {
    this->foreground = c;
    this->invalidate();
    emit foregroundColorChanged();
  }

  const QColor &
  getForegroundColor(void) const
  {
    return this->foreground;
  }

  void setHistorySize(unsigned int length);
  void feed(const SUCOMPLEX *samples, unsigned int length);

  void draw(void);
  void paint(void);

  TVDisplay(QWidget *parent = nullptr);

signals:
  void orderHintChanged();
  void backgroundColorChanged();
  void foregroundColorChanged();
  void axesColorChanged();
  void axesUpdated();

};

#endif
