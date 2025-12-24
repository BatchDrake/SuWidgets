//
//    LCD.cpp: Liquid Crystal Display (7-segment display)
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
#include "LCD.h"

#include <QPainter>
#include <QPainterPath>
#include <SuWidgetsHelpers.h>

void
LCD::recalculateDisplayData()
{
  int width, height;
  unsigned int i, j, k;
  static const unsigned int digit_masks[12] = {
       /* 0 */ ~LCD_SEG_MIDDLE,
       /* 1 */ LCD_SEG_TOP_RIGHT | LCD_SEG_BOTTOM_RIGHT,
       /* 2 */ ~LCD_SEG_TOP_LEFT & ~LCD_SEG_BOTTOM_RIGHT,
       /* 3 */ ~LCD_SEG_TOP_LEFT & ~LCD_SEG_BOTTOM_LEFT,
       /* 4 */ LCD_SEG_TOP_RIGHT | LCD_SEG_BOTTOM_RIGHT | LCD_SEG_TOP_LEFT | LCD_SEG_MIDDLE,
       /* 5 */ ~LCD_SEG_TOP_RIGHT & ~LCD_SEG_BOTTOM_LEFT,
       /* 6 */ ~LCD_SEG_TOP_RIGHT,
       /* 7 */ LCD_SEG_TOP_LEFT | LCD_SEG_TOP | LCD_SEG_TOP_RIGHT | LCD_SEG_BOTTOM_RIGHT,
       /* 8 */ LCD_SEG_ALL_H | LCD_SEG_ALL_V,
       /* 9 */ ~LCD_SEG_BOTTOM_LEFT,
       /* - */ LCD_SEG_MIDDLE,
       /*   */ 0
   };

  static const struct { bool horiz; qreal x; qreal y; } offsets[] = {
      {true, 0, 0},
      {true, 0, 1},
      {true, 0, 2},
      {false,  0, 0},
      {false,  0, 1},
      {false,  1, 0},
      {false,  1, 1}
  };

  QPolygonF seg;
  QBrush brush;

  brush.setStyle(Qt::SolidPattern);

  width  = m_geometry.width();
  height = m_geometry.height();

  // Cache data
  m_width = width;
  m_height = height;

  // Segment width is calculated based on display height and margin configuration
  m_segBoxLength    = .5 * m_height  * m_zoom;
  m_segBoxThickness = m_segBoxLength * m_thickness;

  m_segLength    = m_segBoxLength    * m_segScale;
  m_segThickness = m_segBoxThickness * m_segScale;

  m_margin = .5 * (m_height - 2 * m_segBoxLength - m_segBoxThickness);

  m_glyphWidth = static_cast<int>(m_segBoxLength + 2 * m_segBoxThickness);

  // Compute segment polygons
  qreal halfLen = .5 * m_segLength;
  qreal lilBit  = .5 * m_segThickness;
  qreal halfBoxLen = .5 * m_segBoxLength;
  qreal lilBoxBit  = m_segBoxThickness;

  QTransform t;

  seg << QPointF(0., -halfLen);
  seg << QPointF(lilBit, -halfLen + lilBit);
  seg << QPointF(lilBit, -lilBit + halfLen);
  seg << QPointF(0, halfLen);
  seg << QPointF(-lilBit, -lilBit + halfLen);
  seg << QPointF(-lilBit, -halfLen + lilBit);

  // Compute glyphs
  for (k = 0; k < 2; ++k) {
    brush.setColor(k == 0 ? m_foreground : m_background);

    for (i = 0; i < 12; ++i) {
      m_glyphs[k][i] = QPixmap(m_glyphWidth, m_glyphWidth * 2);

      QPainter painter(&m_glyphs[k][i]);

      painter.setRenderHint(QPainter::Antialiasing);
      painter.fillRect(
            0,
            0,
            m_glyphWidth,
            m_glyphWidth * 2,
            k == 0 ? m_background : m_foreground);

      for (j = 0; j < 7; ++j) {
        if ((digit_masks[i] & (1 << j)) != 0) {
          QTransform t;

          if (offsets[j].horiz) {
            t.translate(
                  m_segBoxLength * offsets[j].x + lilBoxBit + halfBoxLen,
                  m_segBoxLength * offsets[j].y + lilBoxBit + .5 * halfBoxLen);
            t.rotate(90);
          } else {
            t.translate(
                  m_segBoxLength * offsets[j].x + lilBoxBit,
                  m_segBoxLength * offsets[j].y + lilBoxBit + 1.5 * halfBoxLen);
          }

          QPolygonF curSeg = t.map(seg);
          QPainterPath path;
          path.addPolygon(curSeg);
          painter.fillPath(path, brush);
        }
      }
    }
  }
}

void
LCD::drawSeparator(QPainter &painter, qreal x, int index)
{
  painter.setBrush(
        index == 0
        ? m_foreground
        : m_background);

  QPainterPath path;
  path.addEllipse(
        x + m_segBoxLength + m_segBoxThickness,
        m_margin + 2 * m_segBoxLength + 1.5 * m_segBoxThickness,
        m_segThickness,
        m_segThickness);
  painter.fillPath(
        path,
        index == 0
          ? m_foreground
          : m_background);
}

void
LCD::drawLockAt(QPainter &painter, int x, bool locked)
{
  const qreal shackleRadius = m_glyphWidth / 5.;
  const qreal shackleThickness = m_glyphWidth / 10.;
  const qreal bodyWidth = 2 * shackleRadius * 1.7;
  const qreal bodyHeight = bodyWidth * .8;
  const qreal shackleSep = .5 * m_glyphWidth - shackleRadius;
  const qreal bodySep    = .5 * (m_glyphWidth - bodyWidth);
  QPen pen;
  QRectF shackleRect(
        x + shackleSep,
        shackleSep,
        2 * shackleRadius,
        2 * shackleRadius);
  QRectF bodyRect(
        x + bodySep,
        shackleRect.y() + shackleRadius + shackleThickness / 2,
        bodyWidth,
        bodyHeight);

  painter.save();

  if (locked)
    painter.setOpacity(1);
  else
    painter.setOpacity(.5);

  pen.setColor(m_foreground);
  pen.setWidthF(shackleThickness);

  painter.setRenderHint(QPainter::Antialiasing);

  painter.setPen(pen);
  painter.drawArc(shackleRect, 0, locked ? 180 * 16 : 150 * 16);

  painter.fillRect(bodyRect, m_foreground);
  painter.restore();

  m_lockRect = QRectF(
        0,
        shackleRect.y(),
        m_glyphWidth,
        bodyHeight + shackleRadius);

  m_haveLockRect = true;
}

void
LCD::drawContent()
{
  QPainter painter(&m_contentPixmap);
  painter.fillRect(0, 0, m_width, m_height, m_background);
  qint64 value = m_value;
  qint64 copy;
  bool negative = false;
  int index;
  qreal x;
  qreal maxX;

  m_digits = 0;

  if (value < 0) {
    value = -value;
    negative = true;
  }

  copy = value;
  while (copy != 0) {
    copy /= 10;
    ++m_digits;
  }

  x = m_width;

  if (m_digits < SCAST(int, m_minDigits))
    m_digits = m_minDigits;

  for (int i = 0; i < m_digits; ++i) {
    x -= m_glyphWidth;
    index = m_selected == i && m_revvideo && hasFocus()
        ? 1
        : 0;

    painter.drawPixmap(
          static_cast<int>(x),
          static_cast<int>(m_margin),
          m_glyphs[index][value % 10]);

    // Draw thousands separator
    if (m_showDecimalSeparator && i % 3 == 0)
      drawSeparator(painter, x, index);

    value /= 10;
  }

  maxX = x;

  if (m_hoverDigit >= 0 && m_hoverDigit >= m_digits && m_digits > 0) {
    int count = m_hoverDigit - m_digits + 1;
    x = m_width - m_glyphWidth * (m_hoverDigit + 1);
    if (x < maxX)
      maxX = x;

    painter.setOpacity(.5);

    for (int i = 0; i < count; ++i) {
      painter.drawPixmap(
            static_cast<int>(x + i * m_glyphWidth),
            static_cast<int>(m_margin),
            m_glyphs[0][0]);

      if (m_showDecimalSeparator && (m_hoverDigit - i) % 3 == 0)
        drawSeparator(painter, x + i * m_glyphWidth, 0);
    }

    painter.setOpacity(1);
  }

  if (hasFocus() && m_selected >= m_digits) {
    x = m_width - m_glyphWidth * (m_selected + 1);
    if (x < maxX)
      maxX = x;

    index = m_revvideo ? 1 : 0;

    painter.drawPixmap(
          static_cast<int>(x),
          static_cast<int>(m_margin),
          m_glyphs[index][11]);

  }

  /* If negative, draw minus sign */
  if (negative) {
    maxX -= m_glyphWidth;
    painter.drawPixmap(
          static_cast<int>(maxX),
          static_cast<int>(m_margin),
          m_glyphs[0][10]);
  }

  if (m_lockStateEditable)
    drawLockAt(painter, 0, m_locked);
  else
    m_haveLockRect = false;
}

void
LCD::draw()
{
  if (m_dirty && m_haveGeometry) {
    if (m_geometryChanged) {
      recalculateDisplayData();
      m_geometryChanged = false;
    }

    drawContent();
    update();

    m_dirty = false;
  }
}

void
LCD::resizeEvent(QResizeEvent *)
{
  if (!size().isValid())
    return;

  if (m_geometry != size()) {
    m_geometry = size();
    m_contentPixmap = QPixmap(m_geometry.width(), m_geometry.height());
    m_geometryChanged = true;
    m_dirty = true;
    m_haveGeometry = true;
    draw();
  }
}

void
LCD::paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  painter.drawPixmap(0, 0, m_contentPixmap);
}

void
LCD::mousePressEvent(QMouseEvent *ev)
{
  if (m_haveLockRect)
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (m_lockRect.contains(ev->position()))
#else
    if (m_lockRect.contains(ev->x(), ev->y()))
#endif
      setLocked(!isLocked());

  if (m_glyphWidth > 0)
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    selectDigit((m_width - ev->position().x()) / m_glyphWidth);
#else
    selectDigit((m_width - ev->x()) / m_glyphWidth);
#endif
}

void
LCD::scrollDigit(int digit, int delta)
{
  qint64 value = m_value;
  bool negative = value < 0;
  qint64 mult = 1;

  if (digit < LCD_MAX_DIGITS) {
    selectDigit(digit);

    if (m_selected >= 0 && !m_locked) {
      for (int i = 0; i < m_selected; ++i)
        mult *= 10;

      if (negative) {
        value = -value;
        delta = -delta;
      }

      value += delta * mult;

      if (negative)
        value = -value;

      setValue(value);
    }
  }
}

void
LCD::wheelEvent(QWheelEvent *ev)
{
  if (m_glyphWidth > 0) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    int x = SCAST(int, ev->position().x());
#else
    int x = ev->x();
#endif // QT_VERSION

    // accumulate small wheel events up to a step
    m_cumWheelDelta += ev->angleDelta().y();
    int numSteps = m_cumWheelDelta / (8*15);
    if (abs(numSteps) == 0)
        return;
    m_cumWheelDelta = 0;

    int digit = (m_width - x) / m_glyphWidth;
    scrollDigit(digit, numSteps > 0 ? 1 : -1);
    ev->accept();
  }
}

void
LCD::mouseMoveEvent(QMouseEvent *event)
{
  QRectF rect = this->rect();

  int digit = -1;

  if (m_haveLockRect) {
    qreal lockSpaceX = m_lockRect.x() + m_lockRect.width();
    rect = QRectF(
          lockSpaceX,
          0,
          rect.width() - lockSpaceX,
          rect.height());
  }

  if (rect.contains(event->pos()))
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    digit = (m_width - event->position().x()) / m_glyphWidth;
#else
    digit = (m_width - event->x()) / m_glyphWidth;
#endif

  if (digit != m_hoverDigit) {
    m_hoverDigit = digit;
    m_dirty = true;
    draw();
  }
}

void
LCD::leaveEvent(QEvent *)
{
  if (m_hoverDigit != -1) {
    m_hoverDigit = -1;
    m_dirty = true;
    draw();
  }
}

void
LCD::keyPressEvent(QKeyEvent *event)
{
  bool changes = true;
  qint64 mult = 1;

  switch (event->key()) {
    case Qt::Key_Right:
      selectDigit(m_selected - 1);
      break;

    case Qt::Key_Left:
      selectDigit(m_selected + 1);
      break;

    case Qt::Key_Up:
      scrollDigit(m_selected, +1);
      break;

    case Qt::Key_Down:
      scrollDigit(m_selected, -1);
      break;

    case Qt::Key_0:
    case Qt::Key_1:
    case Qt::Key_2:
    case Qt::Key_3:
    case Qt::Key_4:
    case Qt::Key_5:
    case Qt::Key_6:
    case Qt::Key_7:
    case Qt::Key_8:
    case Qt::Key_9:
      if (m_selected != -1 && !m_locked) {
        qint64 value = m_value;
        bool negative = value < 0;

        if (negative)
          value = -value;

        for (int i = 0; i < m_selected; ++i)
          mult *= 10;

        // Put zero in this position
        value -= ((value / mult) % 10) * mult;
        value += (event->key() - Qt::Key_0) * mult;

        if (negative)
          value = -value;

        setValue(value);

        selectDigit(m_selected - 1);
      }
      break;

    case Qt::Key_Plus:
      if (!m_locked)
        setValue(std::abs(m_value));
      break;

    case Qt::Key_Minus:
      if (!m_locked)
        setValue(-m_value);
      break;

    case Qt::Key_L:
      setLocked(!isLocked());
      break;

    default:
      changes = false;
  }

  if (changes) {
    m_revvideo = true;
    m_dirty = true;
    draw();
  }
}

void
LCD::onTimerTimeout()
{
  m_revvideo = !m_revvideo;
  m_dirty = true;
  draw();
}

LCD::LCD(QWidget *parent) :
  QFrame(parent)
{
  m_contentPixmap = QPixmap(0, 0);
  setFocusPolicy(Qt::StrongFocus);
  m_background = LCD_DEFAULT_BACKGROUND_COLOR;
  m_foreground = LCD_DEFAULT_FOREGROUND_COLOR;
  setMouseTracking(true);

  m_timer = new QTimer(this);
  connect(m_timer, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));
  m_timer->start(LCD_BLINKING_INTERVAL);
}
