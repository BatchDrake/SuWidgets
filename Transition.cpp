//
//    Transition.cpp: Transition graph
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

#include "Transition.h"

#include <QPainter>
#include <assert.h>

#define CROSS_MARK_REL_DIM .1f

QPoint
Transition::floatToScreenPoint(float x, float y)
{
  QPoint qp(
    this->ox + static_cast<int>(.707f * this->width  * this->zoom * x),
    this->oy - static_cast<int>(.707f * this->height * this->zoom * y));

  return qp;
}

void
Transition::drawMarkerAt(QPainter &painter, float x, float y)
{
  QPen pen(this->axes);
  float dim = this->bits <= 3
      ? CROSS_MARK_REL_DIM
      : (CROSS_MARK_REL_DIM / (1 << (this->bits - 3)));

  pen.setStyle(Qt::SolidLine);
  painter.setPen(pen);

  painter.drawLine(
        this->floatToScreenPoint(x - dim, y - dim),
        this->floatToScreenPoint(x + dim, y + dim));

  painter.drawLine(
        this->floatToScreenPoint(x + dim, y - dim),
        this->floatToScreenPoint(x - dim, y + dim));
}

void
Transition::recalculateDisplayData(void)
{
  int width, height;

  width  = this->geometry.width();
  height = this->geometry.height();

  // Cache data
  this->width = width;
  this->height = height;

  // Recalculate origin
  this->ox = width / 2;
  this->oy = height / 2;
}

void
Transition::drawAxes(void)
{
  QPainter painter(&this->axesPixmap);
  QPen pen(this->axes);

  painter.fillRect(0, 0, this->width, this->height, this->background);

  pen.setStyle(Qt::DotLine);
  painter.setPen(pen);

  // Horizontal axis (real)
  painter.drawLine(
        QPoint(0, this->height >> 1),
        QPoint(this->width - 1, this->height >> 1));

  // Vertical axis (imaginary)
  painter.drawLine(
        QPoint(this->width >> 1, 0),
        QPoint(this->width >> 1, this->height - 1));

  // Transition hint
  if (this->bits != 0) {
    auto states = 1 << this->bits;

    SUCOMPLEX angle = 2 * M_PI / states;
    SUCOMPLEX delta = SU_C_EXP(I * angle);
    SUCOMPLEX curr = SU_C_EXP(I * .5f * angle);

    for (int i = 0; i < states; ++i) {
      this->drawMarkerAt(painter, SU_C_REAL(curr), SU_C_IMAG(curr));
      curr *= delta;
    }

  }

  this->axesDrawn = true;
}

void
Transition::drawTransition(void)
{
  if (this->amount > 1) {
    QPainter painter(&this->contentPixmap);
    QColor fg = this->foreground;
    Symbol old, current;
    unsigned int p = 0;
    unsigned int q;
    unsigned int states = 1 << this->bits;
    unsigned int transMtxLen = states * states;
    unsigned int max = 0;
    unsigned long size = this->history.size();
    unsigned int times;
    SUCOMPLEX c1, c2, step, omega;
    float alphaK;

    // Yay more Cobol
    step = I * static_cast<SUCOMPLEX>(M_PI) / static_cast<SUFLOAT>(states);
    omega = 2.f * step;

    if (transMtxLen != this->transMtx.size())
      this->transMtx.resize(transMtxLen);

    std::fill(this->transMtx.begin(), this->transMtx.end(), 0);

    q = this->ptr;

    assert(this->amount <= size);

    // Compose transition matrix
    assert(q < size);
    old = this->history[q++];
    while (++p < this->amount) {
      if (q == size)
        q = 0;

      assert(q < size);
      current = this->history[q++];

      times = ++this->transMtx[current + old * states];
      if (times > max)
        max = times;

      old = current;
    }

    // Draw transitions
    painter.setPen(Qt::RoundCap);
    for (unsigned int j = 0; j < states; ++j) {
      c2 = SU_C_EXP(static_cast<SUFLOAT>(j) * omega + step);
      for (unsigned int i = 0; i < states; ++i) {
        times = this->transMtx[i + j * states];
        alphaK = 255.f * times / static_cast<float>(max);
        c1 = SU_C_EXP(static_cast<SUFLOAT>(i) * omega + step);

        fg.setAlpha(static_cast<int>(alphaK));
        painter.setPen(fg);
        painter.drawLine(
              this->floatToScreenPoint(SU_C_REAL(c1), SU_C_IMAG(c1)),
              this->floatToScreenPoint(SU_C_REAL(c2), SU_C_IMAG(c2)));
      }
    }

  }
}


void
Transition::draw(void)
{
  if (!this->size().isValid())
    return;

  if (this->geometry != this->size()) {
    this->geometry = this->size();
    this->haveGeometry = true;
    this->contentPixmap  = QPixmap(this->geometry.width(), this->geometry.height());
    this->axesPixmap = QPixmap(this->geometry.width(), this->geometry.height());
    this->axesDrawn = false;
  }

  if (!this->axesDrawn) {
    this->recalculateDisplayData();
    this->drawAxes();
    emit this->axesUpdated();
  }

  /* Dump to content pixmap */
  this->contentPixmap = this->axesPixmap.copy(
        0,
        0,
        this->geometry.width(),
        this->geometry.height());

  this->drawTransition();
}


void
Transition::paint(void)
{
  QPainter painter(this);
  painter.drawPixmap(0, 0, this->contentPixmap);
}

void
Transition::setHistorySize(unsigned int length)
{
  this->history.resize(length);
  this->amount = 0;
  this->ptr = 0;
}

void
Transition::feed(const Symbol *samples, unsigned int length)
{
  unsigned int p = 0;
  unsigned int size = static_cast<unsigned int>(this->history.size());
  unsigned int chunk;

  if (length > size) {
    p = length - size;
    length = size;
  }

  while (length > 0) {
    chunk = size - this->ptr;
    if (chunk > length)
      chunk = length;

    memcpy(
          &this->history[this->ptr],
        &samples[p],
        chunk * sizeof(Symbol));

    p         += chunk;
    length    -= chunk;
    this->ptr += chunk;

    if (this->amount < size) {
      this->amount += chunk;
      if (this->amount > size)
        this->amount = size;
    }

    if (this->ptr == size)
      this->ptr = 0;
  }

  assert(this->ptr < size);
  this->invalidate();
}

Transition::Transition(QWidget *parent) :
  ThrottleableWidget(parent)
{
  this->contentPixmap = QPixmap(0, 0);
  this->axesPixmap = QPixmap(0, 0);

  this->history.resize(TRANSITION_DEFAULT_HISTORY_SIZE);

  this->background = TRANSITION_DEFAULT_BACKGROUND_COLOR;
  this->foreground = TRANSITION_DEFAULT_FOREGROUND_COLOR;
  this->axes       = TRANSITION_DEFAULT_AXES_COLOR;

  this->invalidate();
}
