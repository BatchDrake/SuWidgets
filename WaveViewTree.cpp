//
//    WaveViewTree.cpp: Compute rescaled views of the same waveform
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

#include "WaveViewTree.h"
#include "WaveWorker.h"
#include <QDeadlineTimer>
#include <sigutils/util/compat-time.h>

#define WAVE_VIEW_TREE_WORKER_PIECE_LENGTH 4096
#define WAVE_VIEW_TREE_FEEDBACK_MS          500
#define WAVE_VIEW_TREE_MIN_PARALLEL_SIZE   WAVE_VIEW_TREE_WORKER_PIECE_LENGTH

WaveWorker::WaveWorker(WaveViewTree *owner, SUSCOUNT since, QObject *parent) :
  QObject(parent)
{
  m_owner = owner;
  m_since = since;
}

WaveWorker::~WaveWorker()
{
}

void
WaveWorker::buildNextView(
    WaveViewTree::iterator p,
    SUSCOUNT start,
    SUSCOUNT end,
    SUFLOAT  wEnd)
{
  WaveViewTree::iterator next = p + 1;
  SUSCOUNT length, nextLength;
  SUFLOAT nextWEnd = 1;
  SUFLOAT currWend = 1;

  start >>= WAVEFORM_BLOCK_BITS;
  start <<= WAVEFORM_BLOCK_BITS;

  if (next == m_owner->end()) {
    m_owner->append(WaveLimitVector());
    next = m_owner->end() - 1;
    p    = next - 1;
    next->resize(1);
  }

  length = p->size();
  nextLength = (length + WAVEFORM_BLOCK_LENGTH - 1) >> WAVEFORM_BLOCK_BITS;

  if (next->size() < nextLength)
    next->resize(nextLength);

  for (auto i = start; i <= end; i += WAVEFORM_BLOCK_LENGTH) {
    const WaveLimits *data = p->data() + i;
    WaveLimits thisLimit;
    quint64 left = MIN(end + 1 - i, WAVEFORM_BLOCK_LENGTH);

    if (i + WAVEFORM_BLOCK_LENGTH > end) {
      currWend = wEnd;
      nextWEnd = SU_ASFLOAT(left) / WAVEFORM_BLOCK_LENGTH;
    }

    WaveViewTree::calcLimitsBlock(thisLimit, data, left, currWend);

    (*next)[i >> WAVEFORM_BLOCK_BITS] = thisLimit;
  }

  if (next->size() > 1)
    buildNextView(
        next,
        start >> WAVEFORM_BLOCK_BITS,
        end   >> WAVEFORM_BLOCK_BITS,
        nextWEnd);
}

void
WaveWorker::build(SUSCOUNT start, SUSCOUNT end)
{
  WaveViewTree::iterator next = m_owner->begin();
  const SUCOMPLEX *data;
  SUSCOUNT length = m_owner->m_length;
  SUSCOUNT nextLength;
  SUFLOAT wEnd = 1;

  start >>= WAVEFORM_BLOCK_BITS;
  start <<= WAVEFORM_BLOCK_BITS;

  if (next == m_owner->end()) {
    m_owner->append(WaveLimitVector());
    next = m_owner->begin();
    next->resize(1);
  }

  nextLength = (length + WAVEFORM_BLOCK_LENGTH - 1) >> WAVEFORM_BLOCK_BITS;

  if (next->size() < nextLength)
    next->resize(nextLength);

  for (SUSCOUNT i = start; i <= end; i += WAVEFORM_BLOCK_LENGTH) {
    WaveLimits thisLimit;
    quint64 left  = MIN(end + 1 - i, WAVEFORM_BLOCK_LENGTH);
    data          = m_owner->m_data + i;

    if (i + WAVEFORM_BLOCK_LENGTH > end)
      wEnd = SU_ASFLOAT(left) / WAVEFORM_BLOCK_LENGTH;

    WaveViewTree::calcLimitsBuf(thisLimit, data, left, start == 0);

    (*next)[i >> WAVEFORM_BLOCK_BITS] = thisLimit;
  }

  if (next->size() > 1)
    buildNextView(
          next,
          start >> WAVEFORM_BLOCK_BITS,
          end   >> WAVEFORM_BLOCK_BITS,
          wEnd);
}

void
WaveWorker::cancel()
{
  QMutexLocker locker(&m_mutex);

  m_cancelFlag = true;
}

void
WaveWorker::run(void)
{
  SUSCOUNT i = m_since;
  SUSCOUNT length;
  struct timeval tv, otv, diff;
  SUSDIFF time_ms;

  gettimeofday(&otv, nullptr);

  while (i < m_owner->m_length && !m_cancelFlag) {
    m_mutex.lock();

    length = WAVE_VIEW_TREE_WORKER_PIECE_LENGTH;
    if (i + length >= m_owner->m_length)
      length = m_owner->m_length - i;

    SuWidgetsHelpers::calcLimits(
          &m_owner->m_oMin,
          &m_owner->m_oMax,
          m_owner->m_data + i,
          length,
          i > 0);

    SuWidgetsHelpers::kahanMeanAndRms(
          &m_owner->m_mean,
          &m_owner->m_rms,
          m_owner->m_data + i,
          length,
          &m_owner->m_state);

    try {
      build(i, i + length - 1);
    } catch (std::bad_alloc &) {
      m_cancelFlag = true;
    }

    gettimeofday(&tv, nullptr);
    timersub(&tv, &otv, &diff);

    time_ms = diff.tv_sec * 1000 + diff.tv_usec / 1000;

    if (time_ms > WAVE_VIEW_TREE_FEEDBACK_MS) {
      otv = tv;
      emit progress(i, m_owner->m_length - 1);
    }

    i += length;
    m_mutex.unlock();
  }

  m_running = false;
  m_finishedCondition.wakeAll();

  if (m_cancelFlag)
    emit cancelled();
  else
    emit finished();
}

void
WaveWorker::wait()
{
  while (m_running) {
    m_mutex.lock();
    m_finishedCondition.wait(&m_mutex, 100);
    m_mutex.unlock();
  }
}

///////////////////////////////// WaveViewTree /////////////////////////////////
WaveViewTree::WaveViewTree(QObject *parent) : QObject(parent)
{
  m_workerThread = new QThread(this);

  m_workerThread->start();
}

WaveViewTree::~WaveViewTree()
{
  if (m_currentWorker != nullptr) {
    m_currentWorker->cancel();
    m_currentWorker->wait();
  }

  m_workerThread->quit();
  m_workerThread->wait();
}

void
WaveViewTree::safeCancel(void)
{
  if (m_currentWorker != nullptr) {
    m_currentWorker->cancel();
    m_currentWorker->wait();
    m_currentWorker->deleteLater();
    m_currentWorker = nullptr;
  }
}

void
WaveViewTree::calcLimitsBlock(
    WaveLimits &thisLimit,
    const WaveLimits *__restrict data,
    size_t len,
    SUFLOAT wEnd)
{
  //
  // wEnd is a last-block completeness factor (or weight). Can be
  // from 1 / BLOCK_LENGTH to 1.
  //

  if (len > 0) {
    SUFLOAT kInv   = 1.f / (SU_ASFLOAT(len) + wEnd - 1);

    if (!thisLimit.isInitialized()) {
      thisLimit.min = data[0].min;
      thisLimit.max = data[0].max;
    }

    for (SUSCOUNT j = 0; j < len; ++j) {
      if (data[j].max.real() > thisLimit.max.real())
        thisLimit.max = data[j].max.real() + thisLimit.max.imag() * SU_I;
      if (data[j].max.imag() > thisLimit.max.imag())
        thisLimit.max = thisLimit.max.real() + data[j].max.imag() * SU_I;

      if (data[j].min.real() < thisLimit.min.real())
        thisLimit.min = data[j].min.real() + thisLimit.min.imag() * SU_I;
      if (data[j].min.imag() < thisLimit.min.imag())
        thisLimit.min = thisLimit.min.real() + data[j].min.imag() * SU_I;

      if (thisLimit.envelope < data[j].envelope)
        thisLimit.envelope = data[j].envelope;

      if (j == len - 1) {
        thisLimit.mean += wEnd * data[j].mean;
        thisLimit.freq += wEnd * data[j].freq;
      } else {
        thisLimit.mean += data[j].mean;
        thisLimit.freq += data[j].freq;
      }
    }

    // Compute mean, mean frequency and finish
    thisLimit.mean *= kInv;
    thisLimit.freq *= kInv;
  }
}

void
WaveViewTree::calcLimitsBuf(
    WaveLimits &thisLimit,
    const SUCOMPLEX *__restrict data,
    size_t len,
    bool first)
{
  if (len > 0) {
    SUFLOAT env2  = 0;
    SUFLOAT kInv  = 1.f / SU_ASFLOAT(len);

    thisLimit.envelope *= thisLimit.envelope;

    if (!thisLimit.isInitialized()) {
      thisLimit.min = data[0];
      thisLimit.max = data[0];
    }

    for (SUSCOUNT j = 0; j < len; ++j) {
      if (data[j].real() > thisLimit.max.real())
        thisLimit.max = data[j].real() + thisLimit.max.imag() * SU_I;
      if (data[j].imag() > thisLimit.max.imag())
        thisLimit.max = thisLimit.max.real() + data[j].imag() * SU_I;

      if (data[j].real() < thisLimit.min.real())
        thisLimit.min = data[j].real() + thisLimit.min.imag() * SU_I;
      if (data[j].imag() < thisLimit.min.imag())
        thisLimit.min = thisLimit.min.real() + data[j].imag() * SU_I;

      env2 = SU_C_REAL(data[j] * SU_C_CONJ(data[j]));
      if (thisLimit.envelope < env2)
        thisLimit.envelope = env2;

      if (!first)
        thisLimit.freq += SU_C_ARG(data[j] * SU_C_CONJ(data[j - 1]));

      thisLimit.mean += data[j];
    }

    thisLimit.freq *= kInv;
    thisLimit.mean *= kInv;
    thisLimit.envelope = sqrt(thisLimit.envelope);
  }
}


void
WaveViewTree::computeLimitsFar(
    WaveViewTree::const_iterator p,
    qint64 start,
    qint64 end,
    WaveLimits &limits) const
{
  qint64 blockStart = (start + WAVEFORM_BLOCK_LENGTH - 1) >> WAVEFORM_BLOCK_BITS;
  qint64 blockEnd   = (end >> WAVEFORM_BLOCK_BITS) - 1;
  WaveLimits newLimits;
  int prefixBlocks;
  int suffixBlocks;
  qint64 centerBlocks;

  SUCOMPLEX mean_p = 0;
  SUCOMPLEX mean_s = 0;
  SUCOMPLEX mean_c = 0;
  SUFLOAT   wInv = 0;

  if (start > end)
    return;

  if (end >= SCAST(qint64, p->size()))
    end = SCAST(qint64, p->size()) - 1;

  prefixBlocks = SCAST(int, (blockStart << WAVEFORM_BLOCK_BITS) - start);
  suffixBlocks = SCAST(int, end - (blockEnd << WAVEFORM_BLOCK_BITS) - 1);
  centerBlocks = ((blockEnd - blockStart + 1) << WAVEFORM_BLOCK_BITS);

  if (blockStart < blockEnd) {
    if (prefixBlocks > 0) {
      calcLimitsBlock(
            limits,
            p->data() + start,
            SCAST(size_t, prefixBlocks));
      mean_p = limits.mean;
      limits.mean = 0;
    }

    if (suffixBlocks > 0) {
      calcLimitsBlock(
            limits,
            p->data() + end + 1 - suffixBlocks,
            SCAST(size_t, suffixBlocks));
      mean_s = limits.mean;
      limits.mean = 0;
    }

    if ((p + 1) != cend()) {
      computeLimitsFar(p + 1, blockStart, blockEnd, limits);
      mean_c = limits.mean;
      limits.mean = 0;
    }

    wInv = 1.f / SCAST(SUFLOAT, prefixBlocks + suffixBlocks + centerBlocks);

    limits.mean += mean_p * SCAST(SUFLOAT, prefixBlocks) * wInv;
    limits.mean += mean_s * SCAST(SUFLOAT, suffixBlocks) * wInv;
    limits.mean += mean_c * SCAST(SUFLOAT, centerBlocks) * wInv;
  } else {
    calcLimitsBlock(
          limits,
          p->data() + start,
          SCAST(size_t, end - start + 1));
  }
}

void
WaveViewTree::computeLimits(qint64 start, qint64 end, WaveLimits &limits) const
{
  qint64 blockStart = (start + WAVEFORM_BLOCK_LENGTH - 1) >> WAVEFORM_BLOCK_BITS;
  qint64 blockEnd   = (end >> WAVEFORM_BLOCK_BITS) - 1;
  WaveLimits newLimits;
  int prefixSamples;
  int suffixSamples;
  qint64 centerSamples;

  SUCOMPLEX mean_p = 0;
  SUCOMPLEX mean_s = 0;
  SUCOMPLEX mean_c = 0;
  SUFLOAT   wInv = 0;

  if (m_length == 0)
    return;

  if (start > end)
    return;

  if (start < 0)
    start = 0;

  if (end >= SCAST(qint64, m_length))
    end = SCAST(qint64, m_length) - 1;

  prefixSamples = SCAST(int, (blockStart << WAVEFORM_BLOCK_BITS) - start);
  suffixSamples = SCAST(int, end - (blockEnd << WAVEFORM_BLOCK_BITS) - 1);
  centerSamples = ((blockEnd - blockStart + 1) << WAVEFORM_BLOCK_BITS);

  if (blockStart < blockEnd) {
    if (prefixSamples > 0) {
      calcLimitsBuf(
            limits,
            m_data + start,
            SCAST(size_t, prefixSamples),
            start == 0);
      mean_p = limits.mean;
    }

    if (suffixSamples > 0) {
      calcLimitsBuf(
            limits,
            m_data + end + 1 - suffixSamples,
            SCAST(size_t, suffixSamples),
            start == 0);
      mean_s = limits.mean;
      limits.mean = 0;
    }

    if (cbegin() != cend()) {
      computeLimitsFar(cbegin(), blockStart, blockEnd, limits);
      mean_c = limits.mean;
      limits.mean = 0;
    }

    wInv = 1.f / SCAST(SUFLOAT, prefixSamples + suffixSamples + centerSamples);

    limits.mean += mean_p * SCAST(SUFLOAT, prefixSamples) * wInv;
    limits.mean += mean_s * SCAST(SUFLOAT, suffixSamples) * wInv;
    limits.mean += mean_c * SCAST(SUFLOAT, centerSamples) * wInv;
  } else {
    calcLimitsBuf(
          limits,
          m_data + start,
          SCAST(size_t, end - start + 1),
          start == 0);
  }
}


bool
WaveViewTree::clear(void)
{
  safeCancel();

  QList<WaveLimitVector>::clear();
  m_state = SuWidgetsHelpers::KahanState();
  m_data = nullptr;
  m_length = 0;
  m_complete = true;

  // This is a reprocessing too
  emit ready();

  return true;
}

bool
WaveViewTree::reprocess(const SUCOMPLEX *data, SUSCOUNT newLength)
{
  WaveWorker *worker = nullptr;
  SUSCOUNT lastLength = m_length;
  SUSCOUNT processLength = 0;

  safeCancel();

  m_data   = data;
  m_length = newLength;

  m_complete = false;

  if (lastLength != newLength) {
    if (newLength == 0) {
      clear();
    } else if (newLength < lastLength) {
      m_state = SuWidgetsHelpers::KahanState();
      worker = new WaveWorker(this, 0);
      processLength = newLength;
    } else {
      worker = new WaveWorker(this, lastLength);
      processLength = newLength - lastLength;
    }

    if (worker != nullptr) {
      if (processLength >= WAVE_VIEW_TREE_MIN_PARALLEL_SIZE) {
        // Too many samples, process in parallel mode
        m_currentWorker = worker;
        m_currentWorker->moveToThread(m_workerThread);

        connect(this,   SIGNAL(triggerWorker()), worker, SLOT(run()));
        connect(worker, SIGNAL(cancelled()), this, SLOT(onWorkerCancelled(void)));
        connect(worker, SIGNAL(finished()), this, SLOT(onWorkerFinished(void)));
        connect(
              worker,
              SIGNAL(progress(quint64, quint64)),
              this,
              SIGNAL(progress(quint64, quint64)));

        emit triggerWorker();
      } else {
        // Only a few samples, process in serial mode
        worker->run();
        m_complete = true;
        delete worker;
        emit ready();
      }
    }
  }

  return true;
}

void
WaveViewTree::onWorkerFinished(void)
{
  m_complete = true;

  if (m_currentWorker != nullptr) {
    m_currentWorker->deleteLater();
    m_currentWorker = nullptr;
  }

  emit ready();
}

void
WaveViewTree::onWorkerCancelled(void)
{
  m_complete = false;

  if (m_currentWorker != nullptr) {
    m_currentWorker->deleteLater();
    m_currentWorker = nullptr;
  }

  emit ready();
}
