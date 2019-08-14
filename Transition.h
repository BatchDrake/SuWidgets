//
//    Transition.h: Transition graph
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
#ifndef TRANSITION_H
#define TRANSITION_H

#include <QFrame>

#include <cmath>
#include <complex.h>
#include <vector>
#include <tgmath.h>
#include "ThrottleableWidget.h"
#include "Decider.h"

#define TRANSITION_DEFAULT_BACKGROUND_COLOR QColor(0,     0,   0)
#define TRANSITION_DEFAULT_FOREGROUND_COLOR QColor(255, 255, 255)
#define TRANSITION_DEFAULT_AXES_COLOR       QColor(128, 128, 128)

#define TRANSITION_DEFAULT_HISTORY_SIZE 256

class Transition : public ThrottleableWidget
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
  QPixmap contentPixmap;
  QPixmap axesPixmap;

  QSize geometry;


  // Data
  std::vector<Symbol> history;
  std::vector<unsigned int> transMtx;

  unsigned int amount = 0;
  unsigned int ptr = 0;

  // Properties
  QColor background;
  QColor foreground;
  QColor axes;
  float zoom = .5f;
  unsigned int bits = 2;
  bool haveGeometry = false;
  bool axesDrawn = false;

  // Cached data
  int ox;
  int oy;
  int width;
  int height;

  // Private methods
  QPoint floatToScreenPoint(float x, float y);

  void recalculateDisplayData(void);

  void drawMarkerAt(QPainter &curr, float x, float y);
  void drawAxes(void);
  void drawTransition(void);

public:
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
      this->invalidate();
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

  void setHistorySize(unsigned int length);
  void feed(const Symbol *samples, unsigned int length);
  void feed(std::vector<Symbol> const &vec)
  {
    this->feed(vec.data(), static_cast<unsigned int>(vec.size()));
  }
  void draw(void);
  void paint(void);

  Transition(QWidget *parent = nullptr);

signals:
  void orderHintChanged();
  void backgroundColorChanged();
  void foregroundColorChanged();
  void axesColorChanged();
  void axesUpdated();

};

#endif
