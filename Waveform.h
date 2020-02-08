//
//    Waveform.h: Time view widget
//    Copyright (C) 2020 Gonzalo José Carracedo Carballal
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
#define WAVEFORM_DEFAULT_SELECTION_COLOR  QColor(0x08, 0x08, 0x08)
#define WAVEFORM_DEFAULT_ENVELOPE_COLOR   QColor(0x3f, 0x3f, 0x00)
#define WAVEFORM_DEFAULT_SUBSEL_COLOR     QColor(0x7f, 0x08, 0x08)
#define WAVEFORM_MAX_ITERS                20

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
      QColor selectionColor
      READ getSelectionColor
      WRITE setSelectionColor
      NOTIFY selectionColorChanged)

  Q_PROPERTY(
      QColor subSelectionColor
      READ getSubSelectionColor
      WRITE setSubSelectionColor
      NOTIFY subSelectionColorChanged)

  Q_PROPERTY(
      QColor envelopeColor
      READ getEnvelopeColor
      WRITE setEnvelopeColor
      NOTIFY envelopeColorChanged)

  Q_PROPERTY(
      qreal sampleRate
      READ getSampleRate
      WRITE setSampleRate
      NOTIFY sampleRateChanged)

  // Properties
  QColor background;
  QColor foreground;
  QColor selection;
  QColor subSelection;
  QColor envelope;
  QColor axes;
  QColor text;
  qreal sampleRate = 1;
  qreal deltaT = 1;

  bool showWaveform = true;
  bool showEnvelope = false;
  bool showPhase = false;
  bool showPhaseDiff = false;
  bool periodicSelection = false;
  bool realComponent = true;

  unsigned int phaseDiffOrigin = 0;
  int divsPerSelection = 1;

  // Palette
  QColor colorTable[256];

  // State
  QSize geometry;
  bool haveGeometry = false;
  bool axesDrawn = false;
  bool waveDrawn = false;
  bool selectionDrawn = false;

  QImage  waveform;
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
  std::vector<SUCOMPLEX> magPhase;
  bool haveMagPhaseInfo = false;

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
  bool  hSelection = false;
  qreal hSelStart = 0;
  qreal hSelEnd   = 0;

  // Vertical selection (int floating point units)
  bool  vSelection = false;
  qreal vSelStart  = 0;
  qreal vSelEnd    = 0;

  // Behavioral properties
  bool autoScroll = false;
  bool autoFitToEnvelope = false;

  void drawHorizontalAxes(void);
  void drawVerticalAxes(void);
  void drawAxes(void);
  void drawWave(void);
  void drawSelection(void);
  void recalculateDisplayData(void);

  static QString formatLabel(qreal value, int digits, QString units = "s");

  inline bool
  somethingDirty(void) const
  {
    return !this->waveDrawn || !this->axesDrawn || !this->selectionDrawn;
  }

protected:
    //re-implemented widget event handlers
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void leaveEvent(QEvent *event) override;

public:

    static inline QString
    formatLabel(qreal value, QString units)
    {
      int digits = 0;

      if (std::fabs(value) > 0)
        digits = static_cast<int>(std::floor(std::log10(std::fabs(value))));

      return formatLabel(value, digits, units);
    }

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

    inline QColor const &
    phaseDiff2Color(SUFLOAT diff) const
    {
      unsigned index = qBound(
            0u,
            static_cast<unsigned>(diff / (2 * PI) * 255),
            255u);

      return this->colorTable[(index + this->phaseDiffOrigin) & 0xff];
    }

  const inline SUCOMPLEX *
  getData(void) const
  {
    return this->data.data();
  }

  size_t
  getDataLength(void) const
  {
    return this->data.length();
  }

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
  setSelectionColor(const QColor &c)
  {
    this->selection = c;
    this->selectionDrawn = false;
    this->invalidate();
    emit textColorChanged();
  }

  const QColor &
  getSelectionColor(void) const
  {
    return this->selection;
  }

  void
  setSubSelectionColor(const QColor &c)
  {
    this->selection = c;
    this->selectionDrawn = false;
    this->invalidate();
    emit textColorChanged();
  }

  const QColor &
  getSubSelectionColor(void) const
  {
    return this->selection;
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
  setEnvelopeColor(const QColor &c)
  {
    this->envelope = c;
    this->axesDrawn = false;
    this->invalidate();
    emit envelopeColorChanged();
  }

  const QColor &
  getEnvelopeColor(void) const
  {
    return this->envelope;
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

  void
  setDivsPerSelection(int divs)
  {
    if (divs < 1)
      divs = 1;
    this->divsPerSelection = divs;
    if (this->hSelection)
      this->selectionDrawn = false;
    this->invalidate();
  }


  inline qint64
  getSampleStart(void) const
  {
    return this->start;
  }

  inline qint64
  getSampleEnd(void) const
  {
    return this->end;
  }

  inline qreal
  getMax(void) const
  {
    return this->max;
  }

  inline qreal
  getMin(void) const
  {
    return this->min;
  }

  inline qreal
  getCursorTime(void) const
  {
    return this->px2t(this->currMouseX);
  }

  void
  setPalette(const QColor *table)
  {
    unsigned int i;

    for (i = 0; i < 256; ++i)
      this->colorTable[i] = table[i];

    if (this->showEnvelope && this->showPhase && this->showPhaseDiff) {
      this->waveDrawn = false;
      this->axesDrawn = false;
      this->invalidate();
    }
  }

  Waveform(QWidget *parent = nullptr);

  void setData(const std::vector<SUCOMPLEX> *);
  void draw(void) override;
  void paint(void) override;
  void zoomHorizontalReset(void); // To show full wave or to sampPerPix = 1
  void zoomHorizontal(qint64 x, qreal amount); // Zoom at point x.
  void zoomHorizontal(qreal tStart, qreal tEnd);
  void zoomHorizontal(qint64, qint64);
  void saveHorizontal(void);
  void scrollHorizontal(qint64 orig, qint64 to);
  void scrollHorizontal(qint64 delta);
  void selectHorizontal(qreal orig, qreal to);
  bool getHorizontalSelectionPresent(void) const;
  qreal getHorizontalSelectionStart(void) const;
  qreal getHorizontalSelectionEnd(void) const;
  void setAutoScroll(bool);

  SUCOMPLEX getMagPhase(qint64 sample);

  void setShowEnvelope(bool);
  void setShowPhase(bool);
  void setShowPhaseDiff(bool);
  void setShowWaveform(bool);
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

  void setPhaseDiffOrigin(unsigned);

  void resetSelection(void);
  void setPeriodicSelection(bool);

  void setRealComponent(bool real);
  void refreshData(void);

signals:
  void backgroundColorChanged();
  void foregroundColorChanged();
  void axesColorChanged();
  void textColorChanged();
  void selectionColorChanged();
  void subSelectionColorChanged();
  void envelopeColorChanged(void);
  void sampleRateChanged();
  void axesUpdated();
  void selectionUpdated();

  void horizontalRangeChanged(qint64 start, qint64 end);
  void verticalRangeChanged(qreal min, qreal max);
  void horizontalSelectionChanged(qreal start, qreal end);
  void verticalSelectionChanged(qreal min, qreal max);
  void hoverTime(qreal);
};

#endif
