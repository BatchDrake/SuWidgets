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
  WaveViewTree  m_ownWaveTree;
  WaveViewTree *m_waveTree = nullptr;

  // Representation properties
  QColor m_foreground;
  int    m_leftMargin = 0;

  // Horizontal zoom
  qint64 m_start = 0;
  qint64 m_end   = 0;

  // Vertical zoom
  qreal m_min = -1;
  qreal m_max = +1;

  // Horizontal units
  qreal  m_t0 = 0;
  qreal  m_sampleRate = 1;
  qreal  m_deltaT = 1;

  // Data-to-screen converstion factors
  qreal m_sampPerPx = 1;
  qreal m_unitsPerPx = 1; // (max - min) / width
  int   m_height = 1;
  int   m_width  = 1;

  // Cached data
  uint64_t m_lastProgressCurr = 0;
  uint64_t m_lastProgressMax = 0;

  // Representation config
  qreal m_phaseDiffContrast = 1;
  unsigned int m_phaseDiffOrigin = 0;

  bool m_realComponent = true;
  bool m_showWaveform  = true;
  bool m_showEnvelope  = false;
  bool m_showPhase     = false;
  bool m_showPhaseDiff = false;
  bool m_pad[3];

  // Color palette
  QColor m_colorTable[256];

  // Private methods
  inline QColor const &
  phaseDiff2Color(qreal diff) const
  {
    unsigned index = static_cast<unsigned>(
            m_phaseDiffContrast * diff / (2. * PI) * 255);

    return m_colorTable[(index + m_phaseDiffOrigin) & 0xff];
  }

  void drawWaveClose(QPainter &painter);
  void drawWaveFar(QPainter &painter, int level);

public:
  // Inlined methods
  inline bool
  isComplete(void) const
  {
    return m_waveTree->isComplete();
  }

  inline bool
  isRunning(void) const
  {
    return m_waveTree->isRunning();
  }

  inline SUCOMPLEX
  getDataMax(void) const
  {
    return m_waveTree->getMax();
  }

  inline SUCOMPLEX
  getDataMin(void) const
  {
    return m_waveTree->getMin();
  }

  inline SUCOMPLEX
  getDataMean(void) const
  {
    return m_waveTree->getMean();
  }

  inline SUFLOAT
  getDataRMS(void) const
  {
    return m_waveTree->getRMS();
  }

  inline qreal
  samp2t(qreal samp) const
  {
    return samp * m_deltaT + m_t0;
  }

  inline qreal
  t2samp(qreal t) const
  {
    return (t - m_t0) * m_sampleRate;
  }

  inline qreal
  px2samp(qreal px) const
  {
    return (px - m_leftMargin) * m_sampPerPx + static_cast<qreal>(m_start);
  }

  inline qreal
  samp2px(qreal samp) const
  {
    return (samp - static_cast<qreal>(m_start)) / m_sampPerPx + m_leftMargin;
  }

  inline qreal
  px2t(qreal px) const
  {
    return samp2t(px2samp(px));
  }

  inline qreal
  t2px(qreal t) const
  {
    return samp2px(t2samp(t));
  }

  inline qreal
  px2value(qreal px) const
  {
    return (m_height - 1 - px) * m_unitsPerPx + m_min;
  }

  inline qreal
  value2px(qreal val) const
  {
    return m_height - 1 - (val - m_min) / m_unitsPerPx;
  }

  inline qreal
  cast(SUCOMPLEX z) const
  {
    return static_cast<qreal>(
          m_realComponent ? SU_C_REAL(z) : SU_C_IMAG(z));
  }

  inline void
  setForeground(QColor color)
  {
    m_foreground = color;
  }

  inline void
  setLeftMargin(int margin) {
    m_leftMargin = margin;
  }

  inline qint64
  getSampleStart(void) const
  {
    return m_start;
  }

  inline qint64
  getSampleEnd(void) const
  {
    return m_end;
  }

  inline qreal
  getMax(void) const
  {
    return m_max;
  }

  inline qreal
  getMin(void) const
  {
    return m_min;
  }

  inline void
  setPalette(const QColor *table)
  {
    unsigned int i;
    for (i = 0; i < 256; ++i)
      m_colorTable[i] = table[i];
  }

  inline qreal
  getViewInterval(void) const
  {
    return (m_end - m_start) * m_deltaT;
  }

  inline qint64
  getViewSampleInterval(void) const
  {
    return m_end - m_start;
  }

  inline qreal
  getViewRange(void) const
  {
    return m_max - m_min;
  }

  inline qreal
  getSamplesPerPixel(void) const
  {
    return m_sampPerPx;
  }

  inline qreal
  getUnitsPerPixel(void) const
  {
    return m_unitsPerPx;
  }

  inline SUSCOUNT
  getLength(void) const
  {
    return m_waveTree->getLength();
  }

  inline void
  setRealComponent(bool real)
  {
    m_realComponent = real;
  }

  inline bool
  isRealComponent() const
  {
    return m_realComponent;
  }

  inline void
  setShowEnvelope(bool show)
  {
    m_showEnvelope = show;
  }

  inline void
  setShowWaveform(bool show)
  {
    m_showWaveform = show;
  }

  inline void
  setShowPhase(bool show)
  {
    m_showPhase = show;
  }

  inline void
  setShowPhaseDiff(bool show)
  {
    m_showPhaseDiff = show;
  }

  inline void
  setPhaseDiffOrigin(unsigned origin)
  {
    m_phaseDiffOrigin = origin & 0xff;
  }

  inline void
  setPhaseDiffContrast(qreal contrast)
  {
    m_phaseDiffContrast = contrast;
  }

  inline void
  setShowWaveForm(bool show)
  {
    m_showWaveform = show;
  }

  inline bool
  isEnvelopeVisible(void) const
  {
    return m_showEnvelope;
  }

  inline bool
  isPhaseEnabled(void) const
  {
    return m_showPhase;
  }

  inline bool
  isPhaseDiffEnabled(void) const
  {
    return m_showPhaseDiff;
  }

  inline qreal
  getSampleRate(void) const
  {
    return m_sampleRate;
  }

  inline qreal
  getDeltaT(void) const
  {
    return m_deltaT;
  }

  inline void
  computeLimits(qint64 start, qint64 end, WaveLimits &limits) const
  {
    m_waveTree->computeLimits(start, end, limits);
  }

  inline int
  width() const
  {
    return m_width;
  }

  inline int
  height() const
  {
    return m_height;
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
