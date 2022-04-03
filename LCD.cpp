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
#include "LCD.h"

#include <QPainter>
#include <QPainterPath>
#include <SuWidgetsHelpers.h>

void
LCD::recalculateDisplayData(void)
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

  width  = this->geometry.width();
  height = this->geometry.height();

  // Cache data
  this->width = width;
  this->height = height;

  // Segment width is calculated based on display height and margin configuration
  this->segBoxLength    = .5 * this->height * this->zoom;
  this->segBoxThickness = this->segBoxLength * this->thickness;

  this->segLength    = this->segBoxLength    * this->segScale;
  this->segThickness = this->segBoxThickness * this->segScale;

  this->margin = .5 * (this->height - 2 * this->segBoxLength - this->segBoxThickness);

  this->glyphWidth = static_cast<int>(this->segBoxLength + 2 * this->segBoxThickness);

  // Compute segment polygons
  qreal halfLen = .5 * this->segLength;
  qreal lilBit  = .5 * this->segThickness;
  qreal halfBoxLen = .5 * this->segBoxLength;
  qreal lilBoxBit  = this->segBoxThickness;

  QTransform t;

  seg << QPointF(0., -halfLen);
  seg << QPointF(lilBit, -halfLen + lilBit);
  seg << QPointF(lilBit, -lilBit + halfLen);
  seg << QPointF(0, halfLen);
  seg << QPointF(-lilBit, -lilBit + halfLen);
  seg << QPointF(-lilBit, -halfLen + lilBit);

  // Compute glyphs
  for (k = 0; k < 2; ++k) {
    brush.setColor(k == 0 ? this->foreground : this->background);

    for (i = 0; i < 12; ++i) {
      this->glyphs[k][i] = QPixmap(this->glyphWidth, this->glyphWidth * 2);

      QPainter painter(&this->glyphs[k][i]);

      painter.setRenderHint(QPainter::Antialiasing);
      painter.fillRect(
            0,
            0,
            this->glyphWidth,
            this->glyphWidth * 2,
            k == 0 ? this->background : this->foreground);

      for (j = 0; j < 7; ++j) {
        if ((digit_masks[i] & (1 << j)) != 0) {
          QTransform t;

          if (offsets[j].horiz) {
            t.translate(
                  this->segBoxLength * offsets[j].x + lilBoxBit + halfBoxLen,
                  this->segBoxLength * offsets[j].y + lilBoxBit + .5 * halfBoxLen);
            t.rotate(90);
          } else {
            t.translate(
                  this->segBoxLength * offsets[j].x + lilBoxBit,
                  this->segBoxLength * offsets[j].y + lilBoxBit + 1.5 * halfBoxLen);
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
        ? this->foreground
        : this->background);

  QPainterPath path;
  path.addEllipse(
        x + this->segBoxLength + this->segBoxThickness,
        this->margin + 2 * this->segBoxLength + 1.5 * this->segBoxThickness,
        this->segThickness,
        this->segThickness);
  painter.fillPath(
        path,
        index == 0
          ? this->foreground
          : this->background);
}

void
LCD::drawLockAt(QPainter &painter, int x, bool locked)
{
  const qreal shackleRadius = this->glyphWidth / 5.;
  const qreal shackleThickness = this->glyphWidth / 10.;
  const qreal bodyWidth = 2 * shackleRadius * 1.7;
  const qreal bodyHeight = bodyWidth * .8;
  const qreal shackleSep = .5 * this->glyphWidth - shackleRadius;
  const qreal bodySep    = .5 * (this->glyphWidth - bodyWidth);
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

  pen.setColor(this->foreground);
  pen.setWidthF(shackleThickness);

  painter.setRenderHint(QPainter::Antialiasing);

  painter.setPen(pen);
  painter.drawArc(shackleRect, 0, locked ? 180 * 16 : 150 * 16);

  painter.fillRect(bodyRect, this->foreground);
  painter.restore();

  this->lockRect = QRectF(
        0,
        shackleRect.y(),
        this->glyphWidth,
        bodyHeight + shackleRadius);

  this->haveLockRect = true;
}

void
LCD::drawContent(void)
{
  QPainter painter(&this->contentPixmap);
  painter.fillRect(0, 0, this->width, this->height, this->background);
  qint64 value = this->value;
  qint64 copy;
  bool negative = false;
  int index;
  qreal x;
  qreal maxX;

  this->digits = 0;

  if (value < 0) {
    value = -value;
    negative = true;
  }

  copy = value;
  while (copy != 0) {
    copy /= 10;
    ++this->digits;
  }

  x = this->width;

  if (this->digits == 0)
    this->digits = 1;

  for (int i = 0; i < this->digits; ++i) {
    x -= this->glyphWidth;
    index = this->selected == i && this->revvideo && this->hasFocus()
        ? 1
        : 0;

    painter.drawPixmap(
          static_cast<int>(x),
          static_cast<int>(this->margin),
          this->glyphs[index][value % 10]);

    // Draw thousands separator
    if (i % 3 == 0)
      this->drawSeparator(painter, x, index);

    value /= 10;
  }

  maxX = x;

  if (this->hoverDigit >= 0
      && this->hoverDigit >= this->digits
      && this->digits > 0) {
    int count = this->hoverDigit - this->digits + 1;
    x = this->width - this->glyphWidth * (this->hoverDigit + 1);
    if (x < maxX)
      maxX = x;

    painter.setOpacity(.5);

    for (int i = 0; i < count; ++i) {
      painter.drawPixmap(
            static_cast<int>(x + i * this->glyphWidth),
            static_cast<int>(this->margin),
            this->glyphs[0][0]);

      if ((this->hoverDigit - i) % 3 == 0)
        this->drawSeparator(painter, x + i * this->glyphWidth, 0);
    }

    painter.setOpacity(1);
  }

  if (this->hasFocus() && this->selected >= this->digits) {
    x = this->width - this->glyphWidth * (this->selected + 1);
    if (x < maxX)
      maxX = x;

    index = this->revvideo ? 1 : 0;

    painter.drawPixmap(
          static_cast<int>(x),
          static_cast<int>(this->margin),
          this->glyphs[index][11]);

  }

  /* If negative, draw minus sign */
  if (negative) {
    maxX -= this->glyphWidth;
    painter.drawPixmap(
          static_cast<int>(maxX),
          static_cast<int>(this->margin),
          this->glyphs[0][10]);
  }

  this->drawLockAt(painter, 0, this->locked);
}

void
LCD::draw(void)
{
  if (this->dirty && this->haveGeometry) {
    if (this->geometryChanged) {
      this->recalculateDisplayData();
      this->geometryChanged = false;
    }

    this->drawContent();
    this->update();

    this->dirty = false;
  }
}

void
LCD::resizeEvent(QResizeEvent *)
{
  if (!this->size().isValid())
    return;

  if (this->geometry != this->size()) {
    this->geometry = this->size();
    this->contentPixmap = QPixmap(this->geometry.width(), this->geometry.height());
    this->geometryChanged = true;
    this->dirty = true;
    this->haveGeometry = true;
    this->draw();
  }
}

void
LCD::paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  painter.drawPixmap(0, 0, this->contentPixmap);
}

void
LCD::mousePressEvent(QMouseEvent *ev)
{
  if (this->haveLockRect)
    if (this->lockRect.contains(ev->x(), ev->y()))
      this->setLocked(!this->isLocked());

  if (this->glyphWidth > 0)
    this->selectDigit((this->width - ev->x()) / this->glyphWidth);
}

void
LCD::scrollDigit(int digit, int delta)
{
  qint64 value = this->value;
  bool negative = value < 0;
  qint64 mult = 1;

  if (digit < LCD_MAX_DIGITS) {
    this->selectDigit(digit);

    if (this->selected >= 0 && !this->locked) {
      for (int i = 0; i < this->selected; ++i)
        mult *= 10;

      if (negative) {
        value = -value;
        delta = -delta;
      }

      value += delta * mult;

      if (negative)
        value = -value;

      this->setValue(value);
    }
  }
}

void
LCD::wheelEvent(QWheelEvent *ev)
{
  if (this->glyphWidth > 0) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    int x = SCAST(int, ev->position().x());
#else
    int x = ev->x();
#endif // QT_VERSION
    int amount = ev->angleDelta().y() > 0 ? 1 : -1;
    int digit = (this->width - x) / this->glyphWidth;
    this->scrollDigit(digit, amount);
    ev->accept();
  }
}

void
LCD::mouseMoveEvent(QMouseEvent *event)
{
  QRectF rect = this->rect();

  int digit = -1;

  if (this->haveLockRect) {
    qreal lockSpaceX = this->lockRect.x() + this->lockRect.width();
    rect = QRectF(
          lockSpaceX,
          0,
          rect.width() - lockSpaceX,
          rect.height());
  }

  if (rect.contains(event->pos()))
    digit = (this->width - event->x()) / this->glyphWidth;

  if (digit != this->hoverDigit) {
    this->hoverDigit = digit;
    this->dirty = true;
    this->draw();
  }
}

void
LCD::leaveEvent(QEvent *)
{
  if (this->hoverDigit != -1) {
    this->hoverDigit = -1;
    this->dirty = true;
    this->draw();
  }
}

void
LCD::keyPressEvent(QKeyEvent *event)
{
  bool changes = true;
  qint64 mult = 1;

  switch (event->key()) {
    case Qt::Key_Right:
      this->selectDigit(this->selected - 1);
      break;

    case Qt::Key_Left:
      this->selectDigit(this->selected + 1);
      break;

    case Qt::Key_Up:
      this->scrollDigit(this->selected, +1);
      break;

    case Qt::Key_Down:
      this->scrollDigit(this->selected, -1);
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
      if (this->selected != -1 && !this->locked) {
        qint64 value = this->value;
        bool negative = value < 0;

        if (negative)
          value = -value;

        for (int i = 0; i < this->selected; ++i)
          mult *= 10;

        // Put zero in this position
        value -= ((value / mult) % 10) * mult;
        value += (event->key() - Qt::Key_0) * mult;

        if (negative)
          value = -value;

        this->setValue(value);

        this->selectDigit(this->selected - 1);
      }
      break;

    case Qt::Key_Plus:
      if (!this->locked)
        this->setValue(std::abs(this->value));
      break;

    case Qt::Key_Minus:
      if (!this->locked)
        this->setValue(-this->value);
      break;

    case Qt::Key_L:
      this->setLocked(!this->isLocked());
      break;

    default:
      changes = false;
  }

  if (changes) {
    this->revvideo = true;
    this->dirty = true;
    this->draw();
  }
}

void
LCD::onTimerTimeout(void)
{
  this->revvideo = !this->revvideo;
  this->dirty = true;
  this->draw();
}

LCD::LCD(QWidget *parent) :
  QFrame(parent)
{
  this->contentPixmap = QPixmap(0, 0);
  this->setFocusPolicy(Qt::StrongFocus);
  this->background = LCD_DEFAULT_BACKGROUND_COLOR;
  this->foreground = LCD_DEFAULT_FOREGROUND_COLOR;
  this->setMouseTracking(true);

  this->timer = new QTimer(this);
  connect(this->timer, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));
  this->timer->start(LCD_BLINKING_INTERVAL);
}
