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

  Q_PROPERTY(
      unsigned minDigits
      READ getMinDigits
      WRITE setMinDigits
      NOTIFY minDigitsChanged)

  Q_PROPERTY(
      bool lockStateEditable
      READ getLockStateEditable
      WRITE setLockStateEditable
      NOTIFY lockStateEditableChanged)

  Q_PROPERTY(
      bool showDecimalSeparator
      READ getShowDecimalSeparator
      WRITE setShowDecimalSeparator
      NOTIFY showDecimalSeparatorChanged)

  // Data properties
  qint64 m_value = 0;
  qint64 m_max = LCD_MAX_DEFAULT;
  qint64 m_min = LCD_MIN_DEFAULT;

  QColor m_background;
  QColor m_foreground;
  qreal  m_zoom = 0.707;
  qreal  m_thickness = LCD_DEFAULT_THICKNESS;
  qreal  m_segScale  = LCD_DEFAULT_SEG_SCALE;

  unsigned m_minDigits = 1;
  bool     m_lockStateEditable = true;
  bool     m_showDecimalSeparator = true;

  // Content pixmap and drawing area
  QPixmap m_contentPixmap;
  QPixmap m_glyphs[2][12];
  QSize   m_geometry;
  bool    m_dirty = false;
  bool    m_geometryChanged = false;
  bool    m_haveGeometry = false;
  bool    m_locked = false;

  // Cached data
  int m_width;
  int m_height;
  int m_glyphWidth;
  int m_glyphHeight;
  int m_cumWheelDelta = 0;

  qreal m_segBoxThickness;
  qreal m_segBoxLength;
  qreal m_segThickness;
  qreal m_segLength;
  qreal m_margin;
  QRectF m_lockRect;
  bool m_haveLockRect = false;

  // Blinking timer
  QTimer *m_timer = nullptr;
  bool m_revvideo = false;
  bool m_pad3[7];
  int m_selected = -1;
  int m_digits = 1;
  int m_hoverDigit = -1;

  // Private methods
  void drawSegAt(int x, int y, bool flip);
  void drawLockAt(QPainter &, int x, bool locked);
  void recalculateDisplayData();
  void drawContent();
  void draw();
  void drawSeparator(QPainter &, qreal x, int index);
  void scrollDigit(int digit, int delta);

public:
  // Unfortunately, we cannot add a "signal = true" parameter to these functions
  // as a specific signature is expected by QtCreator. Instead, we offer the
  // "silent" versions of them.

  bool
  setValueSilent(qint64 value) {
    if (value > m_max)
      value = m_max;
    else if (value < m_min)
      value = m_min;

    if (m_value != value) {
      m_value = value;
      m_dirty = true;
      draw();
      return true;
    }

    return false;
  }

  void
  setValue(qint64 value) {
    if (setValueSilent(value))
      emit valueChanged();
  }


  qint64
  getValue() const {
    return m_value;
  }

  void
  getValue(qint64 &value) const {
    value = m_value;
  }

  bool
  isLocked() const {
    return m_locked;
  }

  void
  setLocked(bool locked) {
    if (m_locked != locked) {
      m_locked = locked;
      m_dirty  = true;
      draw();
      emit lockStateChanged();
    }
  }

  bool
  getShowDecimalSeparator() const {
    return m_showDecimalSeparator;
  }

  void
  setShowDecimalSeparator(bool show) {
    if (m_showDecimalSeparator != show) {
      m_showDecimalSeparator = show;
      m_dirty  = true;
      draw();
      emit showDecimalSeparatorChanged();
    }
  }


  bool
  setMaxSilent(qint64 max) {
    auto value = m_value;
    if (max < m_min)
      max = m_min;

    if (value > m_max)
      value = m_max;

    m_max = max;

    if (m_value != value) {
      m_value = value;
      m_dirty = true;
      draw();
      return true;
    }

    return false;
  }

  void
  setMax(qint64 max) {
    if (setMaxSilent(max))
      emit valueChanged();
  }

  qint64
  getMax() const {
    return m_max;
  }

  void
  getMax(qint64 &max) const {
    max = m_max;
  }

  bool
  setMinSilent(qint64 min) {
    auto value = m_value;
    if (min > m_max)
      min = m_max;

    if (value < m_min)
      value = m_min;

    m_min = min;

    if (m_value != value) {
      m_value = value;
      m_dirty = true;
      draw();
      return true;
    }

    return false;
  }

  void
  setMin(qint64 min) {
    if (setMinSilent(min))
      emit valueChanged();
  }

  qint64
  getMin() const {
    return m_min;
  }

  void
  getMin(qint64 &min) const {
    min = m_min;
  }

  void
  setMinDigits(unsigned minDigits) {
    if (minDigits < 1)
      minDigits = 1;

    if (m_minDigits != minDigits) {
      m_minDigits = minDigits;
      draw();
      emit minDigitsChanged();
    }
  }

  unsigned
  getMinDigits() const {
    return m_minDigits;
  }

  void
  getMinDigits(unsigned &minDigits) const {
    minDigits = m_minDigits;
  }

  void
  setLockStateEditable(bool editable) {
    if (m_lockStateEditable != editable) {
      m_lockStateEditable = editable;
      draw();
      emit lockStateEditableChanged();
    }
  }

  bool
  getLockStateEditable() const {
    return m_lockStateEditable;
  }

  void
  getLockStateEditable(bool &editable) const {
    editable = m_lockStateEditable;
  }

  void
  setZoom(qreal zoom) {
    if (fabs(m_zoom - zoom) >= 1e-8) {
      m_zoom = zoom;
      m_dirty = true;
      m_geometryChanged = true;
      draw();
      emit zoomChanged();
    }
  }

  qreal
  getZoom() const {
    return m_zoom;
  }

  void
  getZoom(qreal &zoom) const {
    zoom = m_zoom;
  }

  void
  setThickness(qreal thickness) {
    if (fabs(m_thickness - thickness) >= 1e-8) {
      m_thickness = thickness;
      m_dirty = true;
      m_geometryChanged = true;
      draw();
      emit thicknessChanged();
    }
  }

  qreal
  getThickness() const {
    return m_thickness;
  }

  void
  getThickness(qreal &thickness) const {
    thickness = m_thickness;
  }

  void
  setSegScale(qreal segScale) {
    if (fabs(m_segScale - segScale) >= 1e-8) {
      m_segScale = segScale;
      m_dirty = true;
      m_geometryChanged = true;
      draw();
      emit segScaleChanged();
    }
  }

  qreal
  getSegScale() const {
    return m_segScale;
  }

  void
  getSegScale(qreal &segScale) const {
    segScale = m_segScale;
  }

  void
  setBackgroundColor(const QColor &c)
  {
    m_background = c;
    m_dirty = true;
    m_geometryChanged = true;
    draw();
    emit backgroundColorChanged();
  }

  const QColor &
  getBackgroundColor() const
  {
    return m_background;
  }

  void
  setForegroundColor(const QColor &c)
  {
    m_foreground = c;
    m_dirty = true;
    m_geometryChanged = true;
    draw();
    emit foregroundColorChanged();
  }

  const QColor &
  getForegroundColor() const
  {
    return m_foreground;
  }

  void
  selectDigit(int digit)
  {
    if (digit < 0)
      m_selected = -1;
    else if (digit >= LCD_MAX_DIGITS)
      m_selected = LCD_MAX_DIGITS - 1;
    else
      m_selected = digit;
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
  void minDigitsChanged();
  void lockStateEditableChanged();
  void showDecimalSeparatorChanged();

public slots:
  void onTimerTimeout();
};

#endif
