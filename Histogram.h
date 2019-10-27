//
//    Histograam.h: Symbol histogram
//    Copyright (C) 2019 Gonzalo José Carracedo Carballal
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
#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <QFrame>

#include <cmath>
#include <complex.h>
#include <vector>
#include <tgmath.h>
#include "ThrottleableWidget.h"
#include "Decider.h"

#define HISTOGRAM_DEFAULT_BACKGROUND_COLOR QColor(0,     0,   0)
#define HISTOGRAM_DEFAULT_FOREGROUND_COLOR QColor(255, 255, 255)
#define HISTOGRAM_DEFAULT_AXES_COLOR       QColor(128, 128, 128)

#define HISTOGRAM_DEFAULT_HISTORY_SIZE 256

class Histogram : public ThrottleableWidget
{
  Q_OBJECT

  Q_PROPERTY(
      unsigned int orderHint
      READ getOrderHint
      WRITE setOrderHint
      NOTIFY orderHintChanged)

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
  QPixmap contentPixmap; // Graph
  QPixmap axesPixmap; // Axes
  QSize geometry;


  // Data
  std::vector<unsigned int> history; // Matches width
  std::vector<float> model; // SNR model
  unsigned int max = 0;
  Decider *decider = nullptr;

  // Properties
  QColor background;
  QColor foreground;
  QColor axes;
  unsigned int bits = 2;
  bool axesDrawn = false;
  bool pad[3];

  // Boundary selection
  float sStart = 0.f; // Relative to width
  float sEnd   = 0.f; // Relative to width
  bool selecting = false;
  bool pad2[7];

  // Cached data
  int ox;
  int oy;
  int width;
  int height;

  // Private methods
  QPoint floatToScreenPoint(float x, float y);

  void recalculateDisplayData(void);

  void drawAxes(void);
  void drawHistogram(void);

public:
  std::vector<unsigned int> const &
  getHistory(void) const
  {
    return this->history;
  }

  void
  setBackgroundColor(const QColor &c)
  {
    this->background = c;
    this->axesDrawn = false;
    this->invalidate();
    emit backgroundColorChanged();
  }

  const QColor &
  getBackgroundColor(void) const
  {
    return this->background;
  }

  void
  setAxesColor(const QColor &c)
  {
    this->axes = c;
    this->axesDrawn = false;
    this->invalidate();
    emit axesColorChanged();
  }

  const QColor &
  getAxesColor(void) const
  {
    return this->axes;
  }

  void
  setForegroundColor(const QColor &c)
  {
    this->foreground = c;
    this->axesDrawn = false;
    this->invalidate();
    emit foregroundColorChanged();
  }

  const QColor &
  getForegroundColor(void) const
  {
    return this->foreground;
  }

  void
  setOrderHint(unsigned int bits)
  {
    if (this->bits != bits) {
      this->bits = bits;
      this->axesDrawn = false;
      this->reset();
      emit orderHintChanged();
    }
  }

  unsigned int
  getOrderHint(void) const
  {
    return this->bits;
  }

  void
  getOrderHint(unsigned int &m_bits) const
  {
    m_bits = this->bits;
  }

  void feed(const SUCOMPLEX *samples, unsigned int length);
  void setSNRModel(std::vector<float> const &model);
  void setDecider(Decider *decider);
  void resetDecider(void);
  void reset(void);

  void draw(void);
  void paint(void);

  void mouseMoveEvent(QMouseEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);

  Histogram(QWidget *parent = nullptr);

signals:
  void orderHintChanged();
  void backgroundColorChanged();
  void foregroundColorChanged();
  void axesColorChanged();
  void axesUpdated();

};

#endif
