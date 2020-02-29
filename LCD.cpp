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
LCD::drawContent(void)
{
  QPainter painter(&this->contentPixmap);
  painter.fillRect(0, 0, this->width, this->height, this->background);
  qint64 value = this->value;
  qint64 copy;
  bool negative = false;
  int index;
  qreal x;

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
    if (i % 3 == 0) {
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

    value /= 10;
  }

  if (this->hasFocus() && this->selected >= this->digits) {
    x -= this->glyphWidth * (this->selected - this->digits + 1);
    index = this->revvideo ? 1 : 0;

    painter.drawPixmap(
          static_cast<int>(x),
          static_cast<int>(this->margin),
          this->glyphs[index][11]);

  }

  /* If negative, draw minus sign */
  if (negative) {
    x -= this->glyphWidth;
    painter.drawPixmap(
          static_cast<int>(x),
          static_cast<int>(this->margin),
          this->glyphs[0][10]);
  }
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

    if (this->selected >= 0) {
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
    int amount = ev->delta() > 0 ? 1 : -1;
    int digit = (this->width - ev->x()) / this->glyphWidth;
    this->scrollDigit(digit, amount);
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
      if (this->selected != -1) {
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

  this->timer = new QTimer(this);
  connect(this->timer, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));
  this->timer->start(LCD_BLINKING_INTERVAL);
}
