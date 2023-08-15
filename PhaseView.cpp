
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

#include "PhaseView.h"

#include <QPainter>
#include <assert.h>
#include "SuWidgetsHelpers.h"

#define PHASE_VIEW_MAG_TICKS 5
#define PHASE_VIEW_ANG_TICKS 48

#define SHRNK .8
#define PHASE_VIEW_ANG_TICK_F1 1.1
#define PHASE_VIEW_ANG_TICK_F2 1.15
#define PHASE_VIEW_TICK_R \
    (.5 * (PHASE_VIEW_ANG_TICK_F1 + PHASE_VIEW_ANG_TICK_F2))

extern QColor yiqTable[];

static inline QColor const &
phaseToColor(qreal angle)
{
  if (angle < 0)
    angle += SCAST(qreal, 2 * PI);

  return yiqTable[
        qBound(
          0,
        SCAST(int, SU_FLOOR(1024 * angle / (2 * PI))),
        1023)];
}

QPoint
PhaseView::floatToScreenPoint(float x, float y)
{
  qreal norm = sqrt(x * x + y * y);

  if (m_zoom * norm > 1) {
    x /= m_zoom * norm;
    y /= m_zoom * norm;
  }

  QPoint qp(
    m_ox + static_cast<int>(.5 * SHRNK * m_width  * m_zoom * x),
    m_oy - static_cast<int>(.5 * SHRNK * m_height * m_zoom * y));

  return qp;
}

void
PhaseView::recalculateDisplayData()
{
  int width, height;

  width  = m_geometry.width();
  height = m_geometry.height();

  // Cache data
  m_width = width;
  m_height = height;

  // Recalculate origin
  m_ox = width / 2;
  m_oy = height / 2;
}

void
PhaseView::drawAxes()
{
  QPainter painter(&m_axesPixmap);
  QPen pen(m_axes);
  QPointF center;
  qreal deltaMag, deltaAng;
  qreal kx, ky;

  center.setX(m_ox);
  center.setY(m_oy);

  deltaMag = 1. / PHASE_VIEW_MAG_TICKS;
  deltaAng = 2 * M_PI / PHASE_VIEW_ANG_TICKS;

  painter.fillRect(0, 0, m_width, m_height, m_background);

  pen.setStyle(Qt::SolidLine);
  painter.setPen(pen);

  // Draw circles
  kx = .5 * SHRNK * deltaMag * m_width * m_zoom;
  ky = .5 * SHRNK * deltaMag * m_height * m_zoom;

  for (int i = 1; i <= PHASE_VIEW_MAG_TICKS; ++i)
    painter.drawEllipse(center, kx * i, ky * i);

  // Draw angular ticks
  painter.save();
  pen.setStyle(Qt::DotLine);
  painter.setPen(pen);

  kx = .5 * SHRNK * m_width;
  ky = .5 * SHRNK * m_height;

  painter.drawEllipse(center, kx * PHASE_VIEW_TICK_R, ky * PHASE_VIEW_TICK_R);

  pen.setStyle(Qt::SolidLine);
  pen.setWidth(qMax(1., .02 * qMin(m_width, m_height)));
  painter.setPen(pen);

  for (int i = 0; i < PHASE_VIEW_ANG_TICKS; ++i) {
    qreal x = .5 * SHRNK * m_width  * cos(i * deltaAng);
    qreal y = .5 * SHRNK * m_height * sin(i * deltaAng);

    qreal x1 = m_ox + PHASE_VIEW_ANG_TICK_F1 * x;
    qreal y1 = m_oy - PHASE_VIEW_ANG_TICK_F1 * y;

    qreal x2 = m_ox + PHASE_VIEW_ANG_TICK_F2 * x;
    qreal y2 = m_oy - PHASE_VIEW_ANG_TICK_F2 * y;

    painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
  }
  painter.restore();


  // In AoA mode, draw normal.
  if (m_aoa) {
    pen.setWidth(2);
    pen.setColor(QColor(255, 0, 0, 127));
    painter.setPen(pen);

    // Forward direction
    painter.drawLine(
          center,
          floatToScreenPoint(0, 1));
    painter.drawLine(
          floatToScreenPoint(0, 1),
          floatToScreenPoint(-.035, .9));
    painter.drawLine(
          floatToScreenPoint(0, 1),
          floatToScreenPoint(+.035, .9));

    // Backward direction
    QColor newColor = m_textColor;
    newColor.setAlpha(127);
    pen.setColor(newColor);
    painter.setPen(pen);
    painter.drawLine(
          center,
          floatToScreenPoint(0, -1));

    painter.drawLine(
          floatToScreenPoint(0, -1),
          floatToScreenPoint(-.035, -.9));
    painter.drawLine(
          floatToScreenPoint(0, -1),
          floatToScreenPoint(+.035, -.9));

    newColor = m_foreground;
    newColor.setAlpha(127);
    pen.setColor(newColor);
    pen.setStyle(Qt::DashLine);
    pen.setWidth(1);
    painter.setPen(pen);

    // Horizontal axis (real)
    painter.drawLine(
          floatToScreenPoint(-1, 0),
          floatToScreenPoint( 0, 0));
    painter.drawLine(
          floatToScreenPoint(+1, 0),
          floatToScreenPoint( 0, 0));
  } else {
    // Horizontal axis (real)
    painter.drawLine(
          QPoint(0, m_height >> 1),
          QPoint(m_width - 1, m_height >> 1));

    // Vertical axis (imaginary)
    painter.drawLine(
          QPoint(m_width >> 1, 0),
          QPoint(m_width >> 1, m_height - 1));

  }

  m_axesDrawn = true;
}

void
PhaseView::drawPhaseView()
{
  QPainter painter(&m_contentPixmap);
  QPointF center;
  SUCOMPLEX c;
  float alphaK;
  unsigned int p = 0;
  qint64 q;
  qint64 skip;
  qint64 size = SCAST(qint64, m_history.size());
  QPen pen;

  center.setX(m_ox);
  center.setY(m_oy);

  pen.setWidth(qMax(1., .02 * qMin(m_width, m_height)));

  if (m_amount > 0) {
    q = SCAST(qint64, m_ptr) - SCAST(qint64, m_amount);
    if (q < 0)
      q += size;

    painter.setPen(Qt::RoundCap);

    alphaK = 1. / size;
    skip = SCAST(unsigned int, size) - m_amount;

    while (p++ < m_amount) {
      c = m_gain * m_history[q];

      QColor fg = phaseToColor(SU_C_ARG(c));
      qreal alpha = alphaK * (p + skip);

      fg.setAlpha(static_cast<int>(255 * alpha * alpha));

      pen.setColor(fg);
      painter.setPen(pen);

      painter.drawLine(
            center,
            floatToScreenPoint(SU_C_REAL(c), SU_C_IMAG(c)));

      if (++q == size)
        q = 0;
    }
  }
}

void
PhaseView::drawAoAView()
{
  QPainter painter(&m_contentPixmap);
  QPointF center;
  SUCOMPLEX c;
  float alphaK;
  qint64 p = 0;
  qint64 q;
  qint64 skip;
  qint64 size = SCAST(qint64, m_history.size());
  QPen pen;
  QColor fg = m_foreground;

  center.setX(m_ox);
  center.setY(m_oy);

  pen.setWidth(1);
  pen.setCapStyle(Qt::RoundCap);

  if (m_amount > 0) {
    q = SCAST(qint64, m_ptr) - SCAST(qint64, m_amount);
    if (q < 0)
      q += size;
    alphaK = 1. / size;
    skip = SCAST(unsigned int, size) - m_amount;

    while (p++ < m_amount) {
      c = m_gain * m_history[q];
      qreal phi   = SU_C_ARG(c);
      qreal mag   = SU_C_ABS(c);
      qreal angle = SU_ASIN(phi / m_phaseScale);
      qreal alpha = alphaK * (p + skip);
      qreal x     = mag * SU_COS(angle);
      qreal y     = mag * SU_SIN(angle);

      // Recall the base ambiguity of angle and pi - angle

      fg.setAlpha(static_cast<int>(255 * pow(alpha, 4)));

      pen.setColor(fg);
      painter.setPen(pen);

      painter.drawLine(
            center,
            floatToScreenPoint(-y, +x));

      painter.drawLine(
            center,
            floatToScreenPoint(-y, -x));

      if (++q == size)
        q = 0;
    }
  }
}



void
PhaseView::draw()
{
  if (!size().isValid())
    return;

  if (m_geometry != size()) {
    m_geometry = size();
    m_haveGeometry = true;
    m_contentPixmap  = QPixmap(m_geometry.width(), m_geometry.height());
    m_axesPixmap = QPixmap(m_geometry.width(), m_geometry.height());
    m_axesDrawn = false;
  }

  if (!m_axesDrawn) {
    recalculateDisplayData();
    drawAxes();
    emit axesUpdated();
  }

  /* Dump to content pixmap */
  m_contentPixmap = m_axesPixmap.copy(
        0,
        0,
        m_geometry.width(),
        m_geometry.height());

  if (m_aoa)
    drawAoAView();
  else
    drawPhaseView();
}


void
PhaseView::paint()
{
  QPainter painter(this);
  painter.drawPixmap(0, 0, m_contentPixmap);
}

void
PhaseView::setPhaseScale(SUFLOAT scale)
{
  m_phaseScale = scale;
}

void
PhaseView::setHistorySize(unsigned int length)
{
  m_history.resize(length);
  m_amount = 0;
  m_ptr = 0;
}

void
PhaseView::feed(const SUCOMPLEX *samples, unsigned int length)
{
  unsigned int p = 0;
  unsigned int size = static_cast<unsigned int>(m_history.size());
  unsigned int chunk;

  if (length > size) {
    p = length - size;
    length = size;
  }

  while (length > 0) {
    chunk = size - m_ptr;
    if (chunk > length)
      chunk = length;

    memcpy(
          &m_history[m_ptr],
        &samples[p],
        chunk * sizeof(SUCOMPLEX));

    p         += chunk;
    length    -= chunk;
    m_ptr += chunk;

    if (m_amount < size) {
      m_amount += chunk;
      if (m_amount > size)
        m_amount = size;
    }

    if (m_ptr == size)
      m_ptr = 0;
  }

  assert(size == 0 || m_ptr < size);
  invalidate();
}

PhaseView::PhaseView(QWidget *parent) :
  ThrottleableWidget(parent)
{
  m_contentPixmap = QPixmap(0, 0);
  m_axesPixmap = QPixmap(0, 0);

  m_history.resize(PhaseView_DEFAULT_HISTORY_SIZE);

  m_background = PhaseView_DEFAULT_BACKGROUND_COLOR;
  m_foreground = PhaseView_DEFAULT_FOREGROUND_COLOR;
  m_textColor  = PhaseView_DEFAULT_TEXT_COLOR;
  m_axes       = PhaseView_DEFAULT_AXES_COLOR;

  invalidate();
}
