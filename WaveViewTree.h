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
#ifndef WAVEVIEWTREE_H
#define WAVEVIEWTREE_H

#include <QObject>
#include <sigutils/types.h>
#include <QList>

#include <vector>
#include <QThread>
#include "SuWidgetsHelpers.h"

#define WAVEFORM_BLOCK_BITS   2
#define WAVEFORM_BLOCK_LENGTH (1 << WAVEFORM_BLOCK_BITS)
#define WAVEFORM_CIRCLE_DIM   4

struct WaveLimits {
  SUCOMPLEX min = +INFINITY + +INFINITY * SU_I;
  SUCOMPLEX max = -INFINITY + -INFINITY * SU_I;
  SUCOMPLEX mean = 0;
  SUFLOAT   envelope = 0;
  SUFLOAT   freq = 0;

  inline bool
  isInitialized(void) const
  {
    return
           isfinite(SU_C_REAL(min))
        && isfinite(SU_C_IMAG(min))
        && isfinite(SU_C_REAL(max))
        && isfinite(SU_C_IMAG(max));
  }
};

typedef std::vector<WaveLimits> WaveLimitVector;

class WaveWorker;

class WaveViewTree : public QObject, public QList<WaveLimitVector> {
  Q_OBJECT

  QThread         *m_workerThread;
  WaveWorker      *m_currentWorker = nullptr;
  const SUCOMPLEX *m_data = nullptr;
  SUSCOUNT         m_length = 0;

  SUCOMPLEX        m_oMin, m_oMax;
  SUCOMPLEX        m_mean;
  SUFLOAT          m_rms;
  SuWidgetsHelpers::KahanState m_state;

  bool             m_complete = true;

  friend class WaveWorker;

  static void calcLimitsBuf(
      WaveLimits &limit,
      const SUCOMPLEX *__restrict buf,
      size_t len,
      bool first = false);

  static void calcLimitsBlock(
      WaveLimits &limit,
      const WaveLimits *__restrict data,
      size_t len,
      SUFLOAT wEnd = 1);

public:
  inline bool
  isComplete(void) const
  {
    return this->m_complete;
  }

  inline bool
  isRunning(void) const
  {
    return this->m_currentWorker != nullptr;
  }

  inline SUCOMPLEX
  getMax(void) const
  {
    return this->m_complete ? m_oMax : 0;
  }

  inline SUCOMPLEX
  getMin(void) const
  {
    return this->m_complete ? m_oMin : 0;
  }

  inline SUCOMPLEX
  getMean(void) const
  {
    return this->m_complete ? m_mean : 0;
  }

  inline SUFLOAT
  getRMS(void) const
  {
    return this->m_complete ? m_rms : 0;
  }

  inline const SUCOMPLEX *
  getData(void) const
  {
    return this->m_data;
  }

  inline SUSCOUNT
  getLength(void) const
  {
    return this->m_length;
  }

  WaveViewTree(QObject *parent = nullptr);
  ~WaveViewTree() override;

  bool reprocess(const SUCOMPLEX *, SUSCOUNT newLength);
  bool clear(void);
  void safeCancel(void);
  void computeLimitsFar(
      WaveViewTree::const_iterator p,
      qint64 start,
      qint64 end,
      WaveLimits &limits) const;
  void computeLimits(qint64 start, qint64 end, WaveLimits &limits) const;

signals:
  void ready(void);
  void triggerWorker(void);
  void progress(quint64, quint64);

public slots:
  void onWorkerFinished(void);
  void onWorkerCancelled(void);
};

#endif // WAVEVIEWTREE_H
