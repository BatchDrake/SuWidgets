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
#include "WaveView.h"
#include <sys/time.h>
#include <QPainterPath>
#include "SuWidgetsHelpers.h"
#include "YIQ.h"

static inline QColor const &
phaseToColor(qreal angle)
{
  if (angle < 0)
    angle += SCAST(qreal, 2 * PI);

  return yiqTable[
        qBound(
          0,
        SCAST(int, SU_FLOOR(1024 * angle / (2 * PI))),
        1023)];
}

WaveView::WaveView()
{
  m_waveTree = &m_ownWaveTree;
  borrowTree(*this);
}

WaveView::~WaveView()
{
  safeCancel();
}

void
WaveView::borrowTree(WaveView &view)
{
  if (m_waveTree != nullptr) {
    disconnect(
          m_waveTree,
          SIGNAL(ready(void)),
          this,
          nullptr);

    disconnect(
          m_waveTree,
          SIGNAL(progress(quint64, quint64)),
          this,
          nullptr);
  }

  m_waveTree = view.m_waveTree;
  connect(
        m_waveTree,
        SIGNAL(ready(void)),
        this,
        SLOT(onReady(void)));

  connect(
        m_waveTree,
        SIGNAL(progress(quint64, quint64)),
        this,
        SLOT(onProgress(quint64, quint64)));
}

qreal
WaveView::getEnvelope(void) const
{
  if (!m_waveTree->isComplete())
    return 0;

  if (m_waveTree->size() == 0)
    return 0;

  return SCAST(qreal, (*m_waveTree)[m_waveTree->size() - 1][0].envelope);
}

void
WaveView::setSampleRate(qreal rate)
{
  m_deltaT     = 1. / rate;
  m_sampleRate = rate;

  // Set horizontal zoom again
  setHorizontalZoom(m_start, m_end);
}

void
WaveView::setTimeStart(qreal t0)
{
  m_t0 = t0;
}

void
WaveView::setHorizontalZoom(qint64 start, qint64 end)
{
  m_start = start;
  m_end   = end;
  setGeometry(m_width, m_height);
}

void
WaveView::setVerticalZoom(qreal min, qreal max)
{
  m_min = min;
  m_max = max;
  setGeometry(m_width, m_height);
}

void
WaveView::setGeometry(int width, int height)
{
  m_width  = width;
  m_height = height;

  m_sampPerPx  = SCAST(qreal, m_end - m_start) / m_width;
  m_unitsPerPx = SCAST(qreal, m_max - m_min)   / m_height;
}

/////////////////////////////// Drawing methods ////////////////////////////////
void
WaveView::drawWaveClose(QPainter &p)
{
  qreal firstSamp, lastSamp;
  qint64 firstIntegerSamp, lastIntegerSamp;
  SUSCOUNT length = m_waveTree->getLength();
  const SUCOMPLEX *data = m_waveTree->getData();
  int prevMinEnvY = 0;
  int prevMaxEnvY = 0;
  int nextX, currX, currY;
  int prevX = 0, prevY = 0;
  int pathX = 0;

  qreal prevPhase = 0;
  qreal alpha = 1;
  bool havePrevEnv = false;
  bool havePrevWf  = false;
  QColor color = m_foreground;
  QColor darkenedForeground;
  int minEnvY = 0, maxEnvY = 0;
  QPen pen;
  bool paintSamples = m_sampPerPx < 1. / (2 * WAVEFORM_CIRCLE_DIM);

  if (m_sampPerPx > 1)
    alpha = sqrt(1. / m_sampPerPx);

  pen.setColor(color);
  pen.setStyle(Qt::SolidLine);
  p.setPen(pen);

  // Initialize darkened foreground
  if (m_showWaveform && !m_showPhase) {
    darkenedForeground.setRed(m_foreground.red() / 3);
    darkenedForeground.setGreen(m_foreground.green() / 3);
    darkenedForeground.setBlue(m_foreground.blue() / 3);
  }

  // Determine the representation range
  firstSamp = px2samp(m_leftMargin);
  lastSamp  = px2samp(m_width - 1);

  firstIntegerSamp = SCAST(qint64, std::ceil(firstSamp));
  lastIntegerSamp  = SCAST(qint64, std::floor(lastSamp));

  if (firstIntegerSamp < 0)
    firstIntegerSamp = 0;

  if (lastIntegerSamp >= SCAST(qint64, length))
    lastIntegerSamp = SCAST(qint64, length - 1);

  nextX = SCAST(int, samp2px(SCAST(qreal, firstIntegerSamp)));
  for (qint64 i = firstIntegerSamp; i <= lastIntegerSamp; ++i) {
    currX = nextX;
    nextX = SCAST(int, samp2px(SCAST(qreal, i + 1)));
    currY = SCAST(int, value2px(cast(data[i])));

    if (i >= 0 && i < SCAST(qint64, length)) {
      // Draw envelope?
      if (m_showEnvelope) {
        // Determine limits
        qreal mag    = SCAST(qreal, SU_C_ABS(data[i]));
        qreal phase  = SCAST(qreal, SU_C_ARG(data[i]));

        int pxLower  = SCAST(int, value2px(+mag));
        int pxUpper  = SCAST(int, value2px(-mag));

        // Previous pixel column is not the same as next
        //  Initialize limits
        if (currX != prevX) {
          minEnvY = pxLower;
          maxEnvY = pxUpper;
          pathX   = prevX;
        } else {
          // If not: update limits
          if (pxLower < minEnvY)
            minEnvY = pxLower;

          if (pxUpper > maxEnvY)
            maxEnvY = pxUpper;
        }

        // Next pixel column will be different: time to draw line
        if (currX != nextX) {
          p.setPen(Qt::NoPen);
          p.setOpacity(m_showWaveform ? .33 : 1.);

          if (havePrevEnv) {
            QPainterPath path;

            if (pathX != currX) {
              path.moveTo(pathX, prevMinEnvY);
              path.lineTo(currX, minEnvY);
              path.lineTo(currX, maxEnvY);
              path.lineTo(pathX, prevMaxEnvY);
            }

            // Show phase?
            if (m_showPhase) {
              if (m_showPhaseDiff) {
                // Display its first derivative (frequency)
                qreal phaseDiff = phase - prevPhase;
                if (phaseDiff < 0)
                  phaseDiff += 2. * SCAST(qreal, PI);
                QColor diffColor = phaseDiff2Color(phaseDiff);

                if (pathX != currX) {
                  p.fillPath(path, diffColor);
                } else {
                  p.setPen(diffColor);
                  p.drawLine(currX, minEnvY, currX, maxEnvY);
                }
              } else {
                // Display it as-is
                if (pathX != currX) {
                  QLinearGradient gradient(prevX, 0, currX, 0);
                  gradient.setColorAt(0, phaseToColor(prevPhase));
                  gradient.setColorAt(1, phaseToColor(phase));
                  p.fillPath(path, gradient);
                } else {
                  p.setPen( phaseToColor(phase));
                  p.drawLine(currX, minEnvY, currX, maxEnvY);
                }

              }
            } else {
              if (pathX != currX) {
                p.fillPath(path, m_foreground);
              } else {
                p.setPen(m_foreground);
                p.drawLine(currX, minEnvY, currX, maxEnvY);
              }
            }
          }

          prevMinEnvY  = minEnvY;
          prevMaxEnvY  = maxEnvY;
          havePrevEnv = true;
        }

        prevPhase  = phase;
      }

      if (m_showWaveform) {
        p.setOpacity(alpha);

        if (havePrevWf) {
          p.setPen(QPen(m_foreground));
          p.drawLine(prevX, prevY, currX, currY);
        }

        if (paintSamples) {
          p.setBrush(QBrush(m_foreground));
          p.drawEllipse(
                currX - WAVEFORM_CIRCLE_DIM / 2,
                currY - WAVEFORM_CIRCLE_DIM / 2,
                WAVEFORM_CIRCLE_DIM,
                WAVEFORM_CIRCLE_DIM);
        }
      }
    }

    prevX      = currX;
    prevY      = currY;
    havePrevWf = true;
  }
}

// Draw wave far away. We will use a different approach: since we already have
// range information, we exploit it in order to know what to paint, and
// how
void
WaveView::drawWaveFar(QPainter &p, int level)
{
  qreal firstSamp, lastSamp;
  qint64 firstBlock, lastBlock;
  int nextX, currX;
  int prevX = -1, prevYA = 0, prevYB = 0;
  int minEnvY = 0, maxEnvY = 0;
  int minWfY  = 0, maxWfY  = 0;
  int bits;
  bool havePrev = false;
  QPen pen;
  WaveViewTree::iterator view = m_waveTree->begin() + level;

  bits = (level + 1) * WAVEFORM_BLOCK_BITS;

  pen.setColor(m_foreground);
  pen.setStyle(Qt::SolidLine);
  p.setPen(pen);

  // Initialize darkened foreground

  // Determine the representation range
  firstSamp = px2samp(m_leftMargin);
  lastSamp  = px2samp(m_width - 1);

  firstBlock = SCAST(qint64, std::ceil(firstSamp)) >> bits;
  lastBlock  = SCAST(qint64, std::floor(lastSamp)) >> bits;

  if (firstBlock < 0)
    firstBlock = 0;

  if (lastBlock >= SCAST(qint64, view->size()))
    lastBlock = SCAST(qint64, view->size() - 1);

  nextX = SCAST(int, samp2px(SCAST(qreal, firstBlock << bits)));

  for (qint64 i = firstBlock; i <= lastBlock; ++i) {
    WaveLimits &z = (*view)[SCAST(unsigned long, i)];
    qint64 samp = i << bits;

    currX = nextX;
    nextX = SCAST(int, samp2px(SCAST(qreal, samp + (1 << bits))));

    // Draw envelope?
    if (m_showEnvelope) {
      // Determine limits
      qreal mag   = SCAST(qreal, z.envelope);
      qreal phase = SCAST(qreal, SU_C_ARG(z.mean));

      int pxHigh  = SCAST(int, value2px(+mag));
      int pxLow   = SCAST(int, value2px(-mag));

      // Previous pixel column is not the same as next
      //  Initialize limits
      if (currX != prevX) {
        minEnvY = pxHigh;
        maxEnvY = pxLow;
      } else {
        // If not: update limits
        if (minEnvY > pxHigh)
          minEnvY = pxHigh;

        if (maxEnvY < pxLow)
          maxEnvY = pxLow;
      }

      // Next pixel column will be different: time to draw line
      if (currX != nextX) {
        p.setPen(Qt::NoPen);
        p.setOpacity(m_showWaveform ? .33 : 1.);

        if (havePrev) {
          // Show phase?
          QColor lineColor;

          if (m_showPhase) {
            // Display its first derivative (frequency, cached)
            if (m_showPhaseDiff) {
              lineColor = phaseDiff2Color(
                    SCAST(qreal, z.freq < 0 ? z.freq + 2 * PI : z.freq));
            }
            else
              lineColor = phaseToColor(phase);
          } else {
            lineColor = m_foreground;
          }

          p.setPen(QPen(lineColor));
          p.drawLine(currX, minEnvY, currX, maxEnvY);
        }
      }
    }

    // Draw waveform
    if (m_showWaveform) {
      qreal min = cast(z.min);
      qreal max = cast(z.max);

      int yA = SCAST(int, value2px(min));
      int yB = SCAST(int, value2px(max));

      // Previous pixel column is not the same as next
      //  Initialize limits
      if (currX != prevX) {
        // These comparisons seem inverted. They are not: remember that
        // vertical screen coordinates are top-down, while regular cartesian
        // coordinates are bottom-up. This results in a change of signedness
        // in the vertical coordinate.

        if (havePrev) {
          minWfY = MIN(yB, prevYA);
          maxWfY = MAX(yA, prevYB);
        } else {
          minWfY = yB;
          maxWfY = yA;
        }
      } else {
        // If not: update limits
        if (minWfY > yB)
          minWfY = yB;

        if (maxWfY < yA)
          maxWfY = yA;
      }

      // Next pixel column is going to be different, draw!
      if (currX != nextX) {
        p.setOpacity(m_showEnvelope ? .33 : .66);
        p.setPen(QPen(m_foreground));
        p.drawLine(currX, minWfY, currX, maxWfY);
      }

      prevYA = yA;
      prevYB = yB;
    }

    prevX    = currX;

    havePrev = true;
  }
}

void
WaveView::drawWave(QPainter &painter)
{
  setGeometry(painter.device()->width(), painter.device()->height());

  if (!m_waveTree->isComplete()) {
    QFont font;
    QFontMetrics metrics(font);
    QString text;
    QRect rect;
    int tw;

    if (m_waveTree->isRunning()) {
      if (m_lastProgressMax > 0)
        text = QString::asprintf(
              "Processing waveform (%ld%% complete)",
              100 * m_lastProgressCurr / m_lastProgressMax);
      else
        text = "Processing waveform";
    } else {
      text = "No wave data";
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    tw = metrics.horizontalAdvance(text);
#else
    tw = metrics.width(text);
#endif // QT_VERSION_CHECK

    rect.setRect(
          m_width / 2 - tw / 2,
          m_height / 2 - metrics.height() / 2,
          tw,
          metrics.height());

    painter.setPen(m_foreground);
    painter.setOpacity(1);
    painter.drawText(rect, Qt::AlignHCenter | Qt::AlignBottom, text);

    return;
  }

  // Nothing to paint? Leave.
  if (m_waveTree->getLength() == 0 || m_waveTree->size() == 0)
    return;

  painter.save();
  if (m_sampPerPx > 8.) {
    int level;

    // More than one waveform block per pixel. The waveform is zoomed out. Compute the
    // zoom-out level. The idea is that:
    //
    // sampPerPx in (0, 8):    Draw lines
    // sampPerPx in [8, 64):   Level 0
    // sampPerPx in [64, 512): Level 1
    //

    level = SCAST(
          int,
          floor(log(m_sampPerPx) / log(WAVEFORM_BLOCK_LENGTH))) - 1;
    if (level >= m_waveTree->size())
      level = m_waveTree->size() - 1;

    drawWaveFar(painter, level);
  } else {
    drawWaveClose(painter);
  }
  painter.restore();
}

void
WaveView::safeCancel()
{
  m_waveTree->safeCancel();
}

void
WaveView::setBuffer(const std::vector<SUCOMPLEX> *buf)
{
  setBuffer(buf->data(), buf->size());
}

void
WaveView::setBuffer(const SUCOMPLEX *data, size_t size)
{
  if (m_waveTree == &m_ownWaveTree) {
    BLOCKSIG(m_waveTree, clear());
    m_waveTree->reprocess(data, size);
  }
}

void
WaveView::refreshBuffer(const std::vector<SUCOMPLEX> *buf)
{
  refreshBuffer(buf->data(), buf->size());
}

void
WaveView::refreshBuffer(const SUCOMPLEX *data, size_t size)
{
  if (m_waveTree == &m_ownWaveTree)
    m_waveTree->reprocess(data, size);
}

///////////////////////////////////// Slots ////////////////////////////////////
void
WaveView::onReady(void)
{
  m_lastProgressCurr = m_lastProgressMax = 0;

  emit ready();
}

void
WaveView::onProgress(quint64 curr, quint64 max)
{
  m_lastProgressCurr = curr;
  m_lastProgressMax  = max;

  emit progress();
}

