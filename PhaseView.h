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
#ifndef PhaseView_H
#define PhaseView_H

#include <QFrame>

#include <cmath>
#include <complex.h>
#include <vector>
#include <tgmath.h>
#include <sigutils/types.h>
#include "ThrottleableWidget.h"

#define PhaseView_DEFAULT_BACKGROUND_COLOR QColor(0,     0,   0)
#define PhaseView_DEFAULT_FOREGROUND_COLOR QColor(255, 255, 255)
#define PhaseView_DEFAULT_AXES_COLOR       QColor(128, 128, 128)

#define PhaseView_DEFAULT_HISTORY_SIZE 256

class PhaseView : public ThrottleableWidget
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


  Q_PROPERTY(
      QColor axesColor
      READ getAxesColor
      WRITE setAxesColor
      NOTIFY axesColorChanged)


  // Drawing area properties
  QPixmap m_contentPixmap;
  QPixmap m_axesPixmap;

  QSize m_geometry;


  // Data
  std::vector<SUCOMPLEX> m_history;
  unsigned int m_amount = 0;
  unsigned int m_ptr = 0;

  // Properties
  QColor m_background;
  QColor m_foreground;
  QColor m_axes;
  float m_zoom = 1.;
  bool m_haveGeometry = false;
  bool m_axesDrawn = false;
  SUFLOAT m_gain = 1.;

  // Cached data
  int m_ox;
  int m_oy;
  int m_width;
  int m_height;

  // Private methods
  QPoint floatToScreenPoint(float x, float y);

  void recalculateDisplayData(void);

  void drawAxes(void);
  void drawPhaseView(void);

public:
  void
  setBackgroundColor(const QColor &c)
  {
    this->m_background = c;
    this->m_axesDrawn = false;
    this->invalidate();
    emit backgroundColorChanged();
  }

  const QColor &
  getBackgroundColor(void) const
  {
    return this->m_background;
  }

  void
  setAxesColor(const QColor &c)
  {
    this->m_axes = c;
    this->m_axesDrawn = false;
    this->invalidate();
    emit axesColorChanged();
  }

  const QColor &
  getAxesColor(void) const
  {
    return this->m_axes;
  }

  void
  setForegroundColor(const QColor &c)
  {
    this->m_foreground = c;
    this->m_axesDrawn = false;
    this->invalidate();
    emit foregroundColorChanged();
  }

  const QColor &
  getForegroundColor(void) const
  {
    return this->m_foreground;
  }

  void
  setGain(SUFLOAT gain)
  {
    this->m_gain = gain;
  }

  SUFLOAT
  getGain(void) const
  {
    return this->m_gain;
  }

  void setHistorySize(unsigned int length);
  void feed(const SUCOMPLEX *samples, unsigned int length);

  void draw(void);
  void paint(void);

  PhaseView(QWidget *parent = nullptr);

signals:
  void orderHintChanged();
  void backgroundColorChanged();
  void foregroundColorChanged();
  void axesColorChanged();
  void axesUpdated();

};

#endif
