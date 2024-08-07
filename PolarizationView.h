//
//    PolarizationView.h: description
//    Copyright (C) 2024 Gonzalo José Carracedo Carballal
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
#ifndef POLARIZATIONVIEW_H
#define POLARIZATIONVIEW_H

#include <QFrame>

#include <cmath>
#include <complex.h>
#include <vector>
#include <tgmath.h>
#include <sigutils/types.h>
#include "ThrottleableWidget.h"

#define PolarizationView_DEFAULT_BACKGROUND_COLOR QColor(0,     0,   0)
#define PolarizationView_DEFAULT_FOREGROUND_COLOR QColor(255, 255, 255)
#define PolarizationView_DEFAULT_TEXT_COLOR       QColor(255, 255, 255)
#define PolarizationView_DEFAULT_AXES_COLOR       QColor(128, 128, 128)

#define PolarizationView_DEFAULT_HISTORY_SIZE 256

class PolarizationView : public ThrottleableWidget
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

  Q_PROPERTY(
      QColor textColor
      READ getTextColor
      WRITE setTextColor
      NOTIFY textColorChanged)


  // Drawing area properties
  QPixmap m_contentPixmap;
  QPixmap m_axesPixmap;

  QSize m_geometry;


  // Jones vectors
  std::vector<SUCOMPLEX> m_vHistory;
  std::vector<SUCOMPLEX> m_hHistory;
  unsigned int m_amount = 0;
  unsigned int m_ptr = 0;

  // Properties
  QColor m_background;
  QColor m_foreground;
  QColor m_axes;
  QColor m_textColor;

  float m_zoom = 1.;
  SUFLOAT m_gain      = 1.;
  SUCOMPLEX m_channelPhase = 1.;

  // Cached data
  int m_ox;
  int m_oy;
  int m_width;
  int m_height;

  bool m_haveGeometry = false;
  bool m_axesDrawn    = false;


  // Private methods
  QPoint floatToScreenPoint(float x, float y);

  void recalculateDisplayData();

  void drawAxes();
  void drawEllipsoid();

public:
  void
  setBackgroundColor(const QColor &c)
  {
    m_background = c;
    m_axesDrawn = false;
    invalidate();
    emit backgroundColorChanged();
  }

  const QColor &
  getBackgroundColor() const
  {
    return m_background;
  }

  void
  setAxesColor(const QColor &c)
  {
    m_axes = c;
    m_axesDrawn = false;
    invalidate();
    emit axesColorChanged();
  }

  const QColor &
  getAxesColor() const
  {
    return m_axes;
  }

  void
  setForegroundColor(const QColor &c)
  {
    m_foreground = c;
    m_axesDrawn = false;
    invalidate();
    emit foregroundColorChanged();
  }

  const QColor &
  getForegroundColor() const
  {
    return m_foreground;
  }

  void
  setTextColor(const QColor &c)
  {
    m_textColor = c;
    m_axesDrawn = false;
    invalidate();
    emit textColorChanged();
  }

  const QColor &
  getTextColor() const
  {
    return m_textColor;
  }

  void
  setGain(SUFLOAT gain)
  {
    m_gain = gain;
  }

  SUFLOAT
  getGain() const
  {
    return m_gain;
  }

  virtual bool hasHeightForWidth() const override;
  virtual int  heightForWidth(int) const override;

  void setChannelPhase(SUFLOAT);
  void setHistorySize(unsigned int length);
  void feed(
      const SUCOMPLEX *hSamp,
      const SUCOMPLEX *vSamp,
      unsigned int length);

  void draw();
  void paint();

  PolarizationView(QWidget *parent = nullptr);

signals:
  void orderHintChanged();
  void backgroundColorChanged();
  void foregroundColorChanged();
  void axesColorChanged();
  void axesUpdated();
  void textColorChanged();
};

#endif // POLARIZATIONVIEW_H
