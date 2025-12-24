//
//    PolarizationView.cpp: Polarization widget
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
#include "PolarizationView.h"

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
PolarizationView::floatToScreenPoint(float x, float y)
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
PolarizationView::recalculateDisplayData()
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
PolarizationView::drawAxes()
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

  // Horizontal axis (real)
  painter.drawLine(
        QPoint(0, m_height >> 1),
        QPoint(m_width - 1, m_height >> 1));

  // Vertical axis (imaginary)
  painter.drawLine(
        QPoint(m_width >> 1, 0),
        QPoint(m_width >> 1, m_height - 1));

  m_axesDrawn = true;
}

//
// It turns out the Jones Vector can be used to determine the polarization
// ellipse of two signals.
//
void
PolarizationView::drawEllipsoid()
{
  QPainter painter(&m_contentPixmap);
  QPointF center;
  SUCOMPLEX Jx, Jy;
  qreal alphaK;
  qint64 p = 0;
  qint64 q;
  qint64 skip;
  qint64 size = SCAST(qint64, m_vHistory.size());
  QPen pen;
  QColor fg = m_foreground;
  QTransform T;

  SUFLOAT maxRadius = SHRNK * fmin(m_width, m_height) * m_zoom;

  center.setX(m_ox);
  center.setY(m_oy);

  pen.setWidth(1);
  pen.setCapStyle(Qt::RoundCap);

  painter.save();

  if (m_amount > 0) {
    q = SCAST(qint64, m_ptr) - SCAST(qint64, m_amount);
    if (q < 0)
      q += size;
    alphaK = 1. / SCAST(qreal, size);
    skip = SCAST(unsigned int, size) - m_amount;

    while (p++ < m_amount) {
      qreal alpha = alphaK * SCAST(qreal, (p + skip));
      Jx = (maxRadius * m_gain) * m_hHistory[q];
      Jy = (maxRadius * m_gain) * m_vHistory[q] * m_channelPhase;


      // Apply transform
      T.setMatrix(
            SU_C_REAL(Jx),  SU_C_REAL(Jy), 0,
            SU_C_IMAG(Jx),  SU_C_IMAG(Jy), 0,
                        0,              0, 1);
      painter.setTransform(QTransform::fromTranslate(m_ox, m_oy));
      painter.setTransform(T, true);

      // Draw a circle.
      fg.setAlpha(static_cast<int>(255 * pow(alpha, 4)));

      pen.setColor(fg);
      pen.setWidth(0);
      painter.setPen(pen);

      painter.drawArc(
            QRectF(-.5, -.5, 1., 1.),
            0,
            360 * 16);

      if (++q == size)
        q = 0;
    }
  }

  painter.restore();
}



void
PolarizationView::draw()
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

  drawEllipsoid();
}


void
PolarizationView::paint()
{
  QPainter painter(this);
  painter.drawPixmap(0, 0, m_contentPixmap);
}

void
PolarizationView::setChannelPhase(SUFLOAT phase)
{
  m_channelPhase = SU_C_EXP(-SU_I * phase);
  invalidate();
}

bool
PolarizationView::hasHeightForWidth() const
{
  return true;
}


int
PolarizationView::heightForWidth(int w) const
{
  return w;
}

void
PolarizationView::setHistorySize(unsigned int length)
{
  m_vHistory.resize(length);
  m_hHistory.resize(length);
  m_amount = 0;
  m_ptr = 0;
}

void
PolarizationView::feed(
    const SUCOMPLEX *hSamp,
    const SUCOMPLEX *vSamp,
    unsigned int length)
{
  unsigned int p = 0;
  unsigned int size = static_cast<unsigned int>(m_vHistory.size());
  unsigned int chunk;

  if (length > size) {
    p = length - size;
    length = size;
  }

  while (length > 0) {
    chunk = size - m_ptr;
    if (chunk > length)
      chunk = length;

    memcpy(&m_hHistory[m_ptr], &hSamp[p], chunk * sizeof(SUCOMPLEX));
    memcpy(&m_vHistory[m_ptr], &vSamp[p], chunk * sizeof(SUCOMPLEX));

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

PolarizationView::PolarizationView(QWidget *parent) :
  ThrottleableWidget(parent)
{
  m_contentPixmap = QPixmap(0, 0);
  m_axesPixmap = QPixmap(0, 0);

  m_hHistory.resize(PolarizationView_DEFAULT_HISTORY_SIZE);
  m_vHistory.resize(PolarizationView_DEFAULT_HISTORY_SIZE);

  m_background = PolarizationView_DEFAULT_BACKGROUND_COLOR;
  m_foreground = PolarizationView_DEFAULT_FOREGROUND_COLOR;
  m_textColor  = PolarizationView_DEFAULT_TEXT_COLOR;
  m_axes       = PolarizationView_DEFAULT_AXES_COLOR;

  invalidate();
}
