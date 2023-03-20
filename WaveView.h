//
//    WaveView.cpp: Draw rescaled views of the same waveform
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

#include <QPainter>
#include <WaveViewTree.h>

class WaveView : public QObject {
  Q_OBJECT

  // Rescaled wave data
  WaveViewTree  ownWaveTree;
  WaveViewTree *waveTree = nullptr;

  // Representation properties
  QColor foreground;
  int    leftMargin = 0;

  // Horizontal zoom
  qint64 start = 0;
  qint64 end   = 0;

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

  // Cached data
  uint64_t lastProgressCurr = 0;
  uint64_t lastProgressMax = 0;

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

  void drawWaveClose(QPainter &painter);
  void drawWaveFar(QPainter &painter, int level);

public:
  // Inlined methods
  inline bool
  isComplete(void) const
  {
    return this->waveTree->isComplete();
  }

  inline bool
  isRunning(void) const
  {
    return this->waveTree->isRunning();
  }

  inline SUCOMPLEX
  getDataMax(void) const
  {
    return this->waveTree->getMax();
  }

  inline SUCOMPLEX
  getDataMin(void) const
  {
    return this->waveTree->getMin();
  }

  inline SUCOMPLEX
  getDataMean(void) const
  {
    return this->waveTree->getMean();
  }

  inline SUFLOAT
  getDataRMS(void) const
  {
    return this->waveTree->getRMS();
  }

  inline qreal
  samp2t(qreal samp) const
  {
    return samp * this->deltaT + this->t0;
  }

  inline qreal
  t2samp(qreal t) const
  {
    return (t - this->t0) * this->sampleRate;
  }

  inline qreal
  px2samp(qreal px) const
  {
    return (px - this->leftMargin) * this->sampPerPx + static_cast<qreal>(this->start);
  }

  inline qreal
  samp2px(qreal samp) const
  {
    return (samp - static_cast<qreal>(this->start)) / this->sampPerPx + this->leftMargin;
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
    return this->waveTree->getLength();
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

  inline void
  computeLimits(qint64 start, qint64 end, WaveLimits &limits) const
  {
    this->waveTree->computeLimits(start, end, limits);
  }


  // Methods
  WaveView();

  qreal getEnvelope(void) const;
  void setSampleRate(qreal rate);
  void setTimeStart(qreal t0);

  void setHorizontalZoom(qint64 start, qint64 end);
  void setVerticalZoom(qreal min, qreal max);
  void setGeometry(int width, int height);
  void borrowTree(WaveView &);
  void drawWave(QPainter &painter);
  void setBuffer(const std::vector<SUCOMPLEX> *);
  void setBuffer(const SUCOMPLEX *, size_t);

  void refreshBuffer(const std::vector<SUCOMPLEX> *);
  void refreshBuffer(const SUCOMPLEX *, size_t);
  // Slots
public slots:
  void onReady(void);
  void onProgress(quint64, quint64);

  // Signals
signals:
  void ready(void);
  void progress(void);
};
#endif // WAVEVIEW_H
