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

struct sigutils_tv_frame_buffer;

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
  QImage  picture;

  QSize geometry;

  // Data
  bool dirty = false;

  // Properties
  QColor background;
  QColor foreground;
  QRgb   colors[2];

public:
  void
  setBackgroundColor(const QColor &c)
  {
    colors[0] = c.rgba();
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
    colors[1] = c.rgba();
    this->foreground = c;
    this->invalidate();
    emit foregroundColorChanged();
  }

  const QColor &
  getForegroundColor(void) const
  {
    return this->foreground;
  }

  bool
  havePicGeometry(void) const
  {
    return this->picture.width() * this->picture.height() > 0;
  }

  QRgb
  tvSampleToRgb(SUFLOAT x)
  {
    x = qBound(SU_ASFLOAT(0), x, SU_ASFLOAT(1));
    return
        qRgba(
          static_cast<int>(
            (1 - x) * qRed(this->colors[0])   + x * qRed(this->colors[1])),
          static_cast<int>(
            (1 - x) * qGreen(this->colors[0]) + x * qGreen(this->colors[1])),
          static_cast<int>(
            (1 - x) * qBlue(this->colors[0])  + x * qBlue(this->colors[1])),
          static_cast<int>(
            (1 - x) * qAlpha(this->colors[0]) + x * qAlpha(this->colors[1])));
  }

  void setPicGeometry(int width, int height);
  void putLine(int line, const SUFLOAT *data, int size);
  void putFrame(const sigutils_tv_frame_buffer *);
  void draw(void);
  void paint(void);

  TVDisplay(QWidget *parent = nullptr);

signals:
  void backgroundColorChanged();
  void foregroundColorChanged();

};

#endif
