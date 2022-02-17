//
//    WaveView.cpp: Compute rescaled views of the same waveform
//    Copyright (C) 2022 Gonzalo José Carracedo Carballal
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
#ifndef WAVEVIEW_H
#define WAVEVIEW_H

#include <sigutils/types.h>
#include <vector>
#include <QList>
#include <QPainter>

struct WaveLimits {
  SUCOMPLEX min = +INFINITY + +INFINITY * I;
  SUCOMPLEX max = -INFINITY + -INFINITY * I;
  SUCOMPLEX mean = 0;
  SUFLOAT   envelope = 0;
  SUFLOAT   freq = 0;
};

#define WAVEFORM_BLOCK_BITS   2
#define WAVEFORM_BLOCK_LENGTH (1 << WAVEFORM_BLOCK_BITS)

typedef std::vector<WaveLimits> WaveLimitVector;
typedef QList<WaveLimitVector> WaveViewList;

class WaveView {
  // Wave data
  const SUCOMPLEX *data = nullptr;
  SUSCOUNT         length = 0;

  // Rescaled wave data
  WaveViewList views;

  // Representation properties
  QColor foreground;
  int    leftMargin = 0;

  // Horizontal zoom
  qint64 start = 0;
  qint64 end   = 0;
  qreal  viewInterval = 0;

  // Vertical zoom
  qreal min = -1;
  qreal max = +1;

  // Horizontal units
  qreal  t0 = 0;
  qreal  sampleRate = 1;
  qreal  deltaT = 1;

  // Data-to-screen converstion factors
  qreal sampPerPx = 1;
  qreal unitsPerPx = 1; // (max - min) / width
  int   height = 1;
  int   width  = 1;

  // Representation config
  qreal phaseDiffContrast = 1;
  unsigned int phaseDiffOrigin = 0;

  bool realComponent = true;
  bool showWaveform  = true;
  bool showEnvelope  = false;
  bool showPhase     = false;
  bool showPhaseDiff = false;
  bool pad[3];

  // Color palette
  QColor colorTable[256];

  // Private methods
  inline QColor const &
  phaseDiff2Color(qreal diff) const
  {
    unsigned index = static_cast<unsigned>(
            this->phaseDiffContrast * diff / (2. * PI) * 255);

    return this->colorTable[(index + this->phaseDiffOrigin) & 0xff];
  }

  void buildNextView(WaveViewList::iterator, SUSCOUNT since);
  void drawWaveClose(QPainter &painter);
  void drawWaveFar(QPainter &painter, int level);

public:
  // Inlined methods
  inline qreal
  samp2t(qreal samp) const
  {
    return (samp + static_cast<qreal>(this->start)) * this->deltaT + this->t0;
  }

  inline qreal
  t2samp(qreal t) const
  {
    return (t - this->t0) * this->sampleRate - static_cast<qreal>(this->start);
  }

  inline qreal
  px2samp(qreal px) const
  {
    return px * this->sampPerPx + static_cast<qreal>(this->start);
  }

  inline qreal
  samp2px(qreal samp) const
  {
    return (samp - static_cast<qreal>(this->start)) / this->sampPerPx;
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
    return (this->height - 1 - px) * this->unitsPerPx + this->min;
  }

  inline qreal
  value2px(qreal val) const
  {
    return this->height - 1 - (val - this->min) / this->unitsPerPx;
  }

  inline qreal
  cast(SUCOMPLEX z) const
  {
    return static_cast<qreal>(
          this->realComponent ? SU_C_REAL(z) : SU_C_IMAG(z));
  }

  inline void
  setForeground(QColor color)
  {
    this->foreground = color;
  }

  inline void
  setLeftMargin(int margin) {
    this->leftMargin = margin;
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

  inline void
  setPalette(const QColor *table)
  {
    unsigned int i;
    for (i = 0; i < 256; ++i)
      this->colorTable[i] = table[i];
  }

  inline qreal
  getViewInterval(void) const
  {
    return (this->end - this->start) * this->deltaT;
  }

  inline qint64
  getViewSampleInterval(void) const
  {
    return this->end - this->start;
  }

  inline qreal
  getViewRange(void) const
  {
    return this->max - this->min;
  }

  inline qreal
  getSamplesPerPixel(void) const
  {
    return this->sampPerPx;
  }

  inline qreal
  getUnitsPerPixel(void) const
  {
    return this->unitsPerPx;
  }

  inline SUSCOUNT
  getLength(void) const
  {
    return this->length;
  }

  inline void
  setRealComponent(bool real)
  {
    this->realComponent = real;
  }

  inline void
  setShowEnvelope(bool show)
  {
    this->showEnvelope = show;
  }

  inline void
  setShowWaveform(bool show)
  {
    this->showWaveform = show;
  }

  inline void
  setShowPhase(bool show)
  {
    this->showPhase = show;
  }

  inline void
  setShowPhaseDiff(bool show)
  {
    this->showPhaseDiff = show;
  }

  inline void
  setPhaseDiffOrigin(unsigned origin)
  {
    this->phaseDiffOrigin = origin & 0xff;
  }

  inline void
  setPhaseDiffContrast(qreal contrast)
  {
    this->phaseDiffContrast = contrast;
  }

  inline void
  setShowWaveForm(bool show)
  {
    this->showWaveform = show;
  }

  inline bool
  isEnvelopeVisible(void) const
  {
    return this->showEnvelope;
  }

  inline bool
  isPhaseEnabled(void) const
  {
    return this->showPhase;
  }

  inline bool
  isPhaseDiffEnabled(void) const
  {
    return this->showPhaseDiff;
  }

  inline qreal
  getSampleRate(void) const
  {
    return this->sampleRate;
  }

  inline qreal
  getDeltaT(void) const
  {
    return this->deltaT;
  }

  // Methods
  void setTimeUnits(qreal t0, qreal rate);
  void setHorizontalZoom(qint64 start, qint64 end);
  void setVerticalZoom(qreal min, qreal max);
  void setGeometry(int width, int height);

  void build(const SUCOMPLEX *data, SUSCOUNT length, SUSCOUNT since = 0);

  void drawWave(QPainter &painter);
};
#endif // WAVEVIEW_H
