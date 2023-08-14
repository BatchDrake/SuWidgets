//
//    LCD.h: Liquid Crystal Display (7-segment display)
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
#ifndef LCD_H
#define LCD_H

#include <QTimer>
#include <QFrame>
#include <QMouseEvent>
#include <cstdint>
#include <cmath>

#define LCD_MAX_DIGITS       11
#define LCD_MAX_DEFAULT      +99999999999ll
#define LCD_MIN_DEFAULT      -99999999999ll
#define LCD_BLINKING_INTERVAL 250
#define LCD_DEFAULT_BACKGROUND_COLOR QColor(0x90, 0xb1, 0x56)
#define LCD_DEFAULT_FOREGROUND_COLOR QColor(   0,    0,    0)
#define LCD_DEFAULT_THICKNESS .2
#define LCD_DEFAULT_SEG_SCALE .9

#define LCD_SEG_TOP          1u
#define LCD_SEG_MIDDLE       2u
#define LCD_SEG_BOTTOM       4u

#define LCD_SEG_ALL_H        7u

#define LCD_SEG_TOP_LEFT     8u
#define LCD_SEG_BOTTOM_LEFT  16u
#define LCD_SEG_TOP_RIGHT    32u
#define LCD_SEG_BOTTOM_RIGHT 64u
#define LCD_SEG_ALL_V        120u

class LCD : public QFrame
{
  Q_OBJECT

  Q_PROPERTY(
      qint64 value
      READ getValue
      WRITE setValue
      NOTIFY valueChanged)

  Q_PROPERTY(
      qreal segScale
      READ getSegScale
      WRITE setSegScale
      NOTIFY segScaleChanged)

  Q_PROPERTY(
      qreal thickness
      READ getThickness
      WRITE setThickness
      NOTIFY thicknessChanged)

  Q_PROPERTY(
      qreal zoom
      READ getZoom
      WRITE setZoom
      NOTIFY zoomChanged)

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
      qint64 min
      READ getMin
      WRITE setMin
      NOTIFY minChanged)

  Q_PROPERTY(
      qint64 max
      READ getMax
      WRITE setMax
      NOTIFY maxChanged)

  // Data properties
  qint64 value = 0;
  qint64 max = LCD_MAX_DEFAULT;
  qint64 min = LCD_MIN_DEFAULT;

  QColor background;
  QColor foreground;
  qreal  zoom = 0.707;
  qreal  thickness = LCD_DEFAULT_THICKNESS;
  qreal  segScale  = LCD_DEFAULT_SEG_SCALE;

  // Content pixmap and drawing area
  QPixmap contentPixmap;
  QPixmap glyphs[2][12];
  QSize   geometry;
  bool    dirty = false;
  bool    geometryChanged = false;
  bool    haveGeometry = false;
  bool    locked = false;

  // Cached data
  int width;
  int height;
  int glyphWidth;
  int glyphHeight;
  int cumWheelDelta = 0;
  qreal segBoxThickness;
  qreal segBoxLength;
  qreal segThickness;
  qreal segLength;
  qreal margin;
  QRectF lockRect;
  bool haveLockRect = false;

  // Blinking timer
  QTimer *timer = nullptr;
  bool revvideo = false;
  bool pad3[7];
  int selected = -1;
  int digits = 1;
  int hoverDigit = -1;

  // Private methods
  void drawSegAt(int x, int y, bool flip);
  void drawLockAt(QPainter &, int x, bool locked);
  void recalculateDisplayData(void);
  void drawContent(void);
  void draw(void);
  void drawSeparator(QPainter &, qreal x, int index);
  void scrollDigit(int digit, int delta);

public:
  // Unfortunately, we cannot add a "signal = true" parameter to these functions
  // as a specific signature is expected by QtCreator. Instead, we offer the
  // "silent" versions of them.

  bool
  setValueSilent(qint64 value) {
    if (value > this->max)
      value = this->max;
    else if (value < this->min)
      value = this->min;

    if (this->value != value) {
      this->value = value;
      this->dirty = true;
      this->draw();
      return true;
    }

    return false;
  }

  void
  setValue(qint64 value) {
    if (this->setValueSilent(value))
      emit valueChanged();
  }


  qint64
  getValue(void) const {
    return this->value;
  }

  void
  getValue(qint64 &m_value) const {
    m_value = this->value;
  }

  bool
  isLocked(void) const {
    return this->locked;
  }

  void
  setLocked(bool locked) {
    if (this->locked != locked) {
      this->locked = locked;
      this->dirty  = true;
      this->draw();
      emit lockStateChanged();
    }
  }

  bool
  setMaxSilent(qint64 max) {
    auto value = this->value;
    if (max < this->min)
      max = this->min;

    if (value > this->max)
      value = this->max;

    this->max = max;

    if (this->value != value) {
      this->value = value;
      this->dirty = true;
      this->draw();
      return true;
    }

    return false;
  }

  void
  setMax(qint64 max) {
    if (this->setMaxSilent(max))
      emit valueChanged();
  }

  qint64
  getMax(void) const {
    return this->max;
  }

  void
  getMax(qint64 &m_max) const {
    m_max = this->max;
  }

  bool
  setMinSilent(qint64 min) {
    auto value = this->value;
    if (min > this->max)
      min = this->max;

    if (value < this->min)
      value = this->min;

    this->min = min;

    if (this->value != value) {
      this->value = value;
      this->dirty = true;
      this->draw();
      return true;
    }

    return false;
  }

  void
  setMin(qint64 min) {
    if (this->setMinSilent(min))
      emit valueChanged();
  }

  qint64
  getMin(void) const {
    return this->min;
  }

  void
  getMin(qint64 &m_min) const {
    m_min = this->min;
  }

  void
  setZoom(qreal zoom) {
    if (fabs(this->zoom - zoom) >= 1e-8) {
      this->zoom = zoom;
      this->dirty = true;
      this->geometryChanged = true;
      this->draw();
      emit zoomChanged();
    }
  }

  qreal
  getZoom(void) const {
    return this->zoom;
  }

  void
  getZoom(qreal &m_zoom) const {
    m_zoom = this->zoom;
  }

  void
  setThickness(qreal thickness) {
    if (fabs(this->thickness - thickness) >= 1e-8) {
      this->thickness = thickness;
      this->dirty = true;
      this->geometryChanged = true;
      this->draw();
      emit thicknessChanged();
    }
  }

  qreal
  getThickness(void) const {
    return this->thickness;
  }

  void
  getThickness(qreal &m_thickness) const {
    m_thickness = this->thickness;
  }

  void
  setSegScale(qreal segScale) {
    if (fabs(this->segScale - segScale) >= 1e-8) {
      this->segScale = segScale;
      this->dirty = true;
      this->geometryChanged = true;
      this->draw();
      emit segScaleChanged();
    }
  }

  qreal
  getSegScale(void) const {
    return this->segScale;
  }

  void
  getSegScale(qreal &segScale) const {
    segScale = this->segScale;
  }

  void
  setBackgroundColor(const QColor &c)
  {
    this->background = c;
    this->dirty = true;
    this->geometryChanged = true;
    this->draw();
    emit backgroundColorChanged();
  }

  const QColor &
  getBackgroundColor(void) const
  {
    return this->background;
  }

  void
  setForegroundColor(const QColor &c)
  {
    this->foreground = c;
    this->dirty = true;
    this->geometryChanged = true;
    this->draw();
    emit foregroundColorChanged();
  }

  const QColor &
  getForegroundColor(void) const
  {
    return this->foreground;
  }

  void
  selectDigit(int digit)
  {
    if (digit < 0)
      this->selected = -1;
    else if (digit >= LCD_MAX_DIGITS)
      this->selected = LCD_MAX_DIGITS - 1;
    else
      this->selected = digit;
  }

  void resizeEvent(QResizeEvent *);
  void paintEvent(QPaintEvent *);
  void mousePressEvent(QMouseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void wheelEvent(QWheelEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void leaveEvent(QEvent *event);

  LCD(QWidget *parent = nullptr);

signals:
  void valueChanged();
  void zoomChanged();
  void thicknessChanged();
  void segScaleChanged();
  void backgroundColorChanged();
  void foregroundColorChanged();
  void maxChanged();
  void minChanged();
  void lockStateChanged();

public slots:
  void onTimerTimeout();
};

#endif
