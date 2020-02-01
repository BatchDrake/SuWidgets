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
#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <QFrame>
#include <QMouseEvent>
#include <QWheelEvent>
#include <sigutils/types.h>
#include "ThrottleableWidget.h"

#define WAVEFORM_DEFAULT_BACKGROUND_COLOR QColor(0x1d, 0x1d, 0x1f)
#define WAVEFORM_DEFAULT_FOREGROUND_COLOR QColor(0xff, 0xff, 0x00)
#define WAVEFORM_DEFAULT_AXES_COLOR       QColor(0x34, 0x34, 0x34)
#define WAVEFORM_DEFAULT_TEXT_COLOR       QColor(0xff, 0xff, 0xff)

class WaveBuffer {
  bool loan = false;
  char pad[7];
  std::vector<SUCOMPLEX> ownBuffer;
  const std::vector<SUCOMPLEX> *buffer = nullptr;

public:
  WaveBuffer();
  WaveBuffer(const std::vector<SUCOMPLEX> *);

  bool feed(SUCOMPLEX val);
  bool feed(std::vector<SUCOMPLEX> const &);
  size_t length(void) const;
  const SUCOMPLEX *data(void) const;
};

class Waveform : public ThrottleableWidget
{
  Q_OBJECT

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


  Q_PROPERTY(
      QColor textColor
      READ getTextColor
      WRITE setTextColor
      NOTIFY textColorChanged)

  Q_PROPERTY(
      qreal sampleRate
      READ getSampleRate
      WRITE setSampleRate
      NOTIFY sampleRateChanged)

  // Properties
  QColor background;
  QColor foreground;
  QColor axes;
  QColor text;
  qreal sampleRate = 1;
  qreal deltaT = 1;

  bool realComponent = true;

  // State
  QSize geometry;
  bool haveGeometry = false;
  bool axesDrawn = false;
  bool waveDrawn = false;
  bool selectionDrawn = false;

  QPixmap contentPixmap; // Data and vertical axes
  QPixmap axesPixmap;    // Only horizontal axes
  QPixmap selectionPixmap;

  // Interactive state
  qreal savedMin;
  qreal savedMax;

  qint64 savedStart;
  qint64 savedEnd;

  qint64 clickX;
  qint64 clickY;

  int  frequencyTextHeight;
  bool frequencyDragging = false;

  int  valueTextWidth;
  bool valueDragging = false;

  bool hSelDragging = false;

  bool haveCursor = false;
  int  currMouseX = 0;

  // Limits
  WaveBuffer data;
  qreal t0 = 0; // Time where the buffer begins

  // Sample limits inside data to show. Please note there are two different
  // paint strategies here. If sampPerPix < 1, we interpolate samples.
  // Otherwise, we need to compute a small histogram of length = height and
  // interpolate it. This histogram es compued by rastering lines instead
  // of points, so we can provide feedback to the user about the amount of
  // data she's missing while keeping zoom continuity.

  qint64 start = 0;
  qint64 end = 0;
  qreal  sampPerPx = 1; // (end - start) / width

  // Tick length (in pixels) in which we place a time mark, starting form t0
  qreal hDivSamples;
  int   hDigits;
  qreal vDivUnits;
  int   vDigits;

  // Level height (in pixels) in which we place a level mark, from 0
  qreal levelHeight = 0;

  // Max limits
  qreal min = -1;
  qreal max =  1;
  qreal unitsPerPx  = 1; // (max - min) / width

  // Horizontal selection (in samples)
  bool   hSelection = false;
  qint64 hSelStart = 0;
  qint64 hSelEnd   = 0;

  // Vertical selection (int floating point units)
  bool  vSelection = false;
  qreal vSelStart  = 0;
  qreal vSelEnd    = 0;

  // Behavioral properties
  bool autoScroll = false;
  bool autoFitToEnvelope = false;
  bool periodicSelection = true;

  inline qreal
  samp2t(qreal samp) const
  {
    return (samp + this->start) * this->deltaT + this->t0;
  }

  inline qreal
  t2samp(qreal t) const
  {
    return (t - this->t0) * this->sampleRate - this->start;
  }

  inline qreal
  px2samp(qreal px) const
  {
    return px * this->sampPerPx + this->start;
  }

  inline qreal
  samp2px(qreal samp) const
  {
    return (samp - this->start) / this->sampPerPx;
  }

  inline qreal
  px2t(qreal px) const
  {
    return this->samp2t(this->px2samp(px));
  }

  inline qreal
  t2px(qreal t) const
  {
    return this->samp2px(this->t2samp(t));
  }

  inline qreal
  px2value(qreal px) const
  {
    return (this->height() - 1 - px) * this->unitsPerPx + this->min;
  }

  inline qreal
  value2px(qreal val) const
  {
    return this->height() - 1 - (val - this->min) / this->unitsPerPx;
  }

  inline qreal
  cast(SUCOMPLEX z) const
  {
    return this->realComponent ? SU_C_REAL(z) : SU_C_IMAG(z);
  }

  void drawHorizontalAxes(void);
  void drawVerticalAxes(void);
  void drawAxes(void);
  void drawWave(void);
  void drawSelection(void);
  void recalculateDisplayData(void);

  static QString formatLabel(qreal value, int digits, QString units = "s");

protected:
    //re-implemented widget event handlers
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void leaveEvent(QEvent *event) override;

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
  setTextColor(const QColor &c)
  {
    this->text = c;
    this->axesDrawn = false;
    this->invalidate();
    emit textColorChanged();
  }

  const QColor &
  getTextColor(void) const
  {
    return this->text;
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

  qreal
  getSampleRate(void) const
  {
    return this->sampleRate;
  }

  void
  setSampleRate(qreal rate)
  {
    if (rate <= 0)
      rate = static_cast<qreal>(std::numeric_limits<SUFLOAT>::epsilon());

    this->sampleRate = rate;
    this->deltaT = 1. / rate;
    this->axesDrawn = false;
    this->invalidate();
    emit sampleRateChanged();
  }

  Waveform(QWidget *parent = nullptr);

  void setData(const std::vector<SUCOMPLEX> *);
  void draw(void) override;
  void paint(void) override;
  void zoomHorizontalReset(void); // To show full wave or to sampPerPix = 1
  void zoomHorizontal(qint64 x, qreal amount); // Zoom at point x.
  void zoomHorizontal(qreal tStart, qreal tEnd);
  void saveHorizontal(void);
  void scrollHorizontal(qint64 orig, qint64 to);
  void scrollHorizontal(qint64 delta);
  void selectHorizontal(qint64 orig, qint64 to);
  bool getHorizontalSelectionPresent(void) const;
  qreal getHorizontalSelectionStart(void) const;
  qreal getHorizontalSelectionEnd(void) const;
  void setAutoScroll(bool);

  void zoomVerticalReset(void);
  void zoomVertical(qint64 y, qreal amount);
  void zoomVertical(qreal start, qreal end);
  void saveVertical(void);
  void scrollVertical(qint64 orig, qint64 to);
  void scrollVertical(qint64 delta);
  void selectVertical(qint64 orig, qint64 to);
  bool getVerticalSelectionPresent(void) const;
  qreal getVerticalSelectionStart(void) const;
  qreal getVerticalSelectionEnd(void) const;
  void fitToEnvelope(void);
  void setAutoFitToEnvelope(bool);

  void resetSelection(void);
  void setPeriodicSelection(bool);

signals:
  void backgroundColorChanged();
  void foregroundColorChanged();
  void axesColorChanged();
  void textColorChanged();
  void sampleRateChanged();
  void axesUpdated();
  void selectionUpdated();
};

#endif
