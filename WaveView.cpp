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

// See https://www.linuxquestions.org/questions/blog/rainbowsally-615861/qt-fast-pixel-color-mixing-35589/

static inline QRgb
qMixRgb(QRgb pixel1, QRgb pixel2, unsigned alpha)
{
  int a, b, c;
  uint res1, res2;

  b = SCAST(int, alpha);
  // Add and sub are faster than a branch too, so we use the compare to 0
  // optimization to get the upper clamp value without branching.  Again,
  // see the disassembly.
  c = b - 255;
  c = (c > 0) ? 0 : c;
  b = c + 255;

  // fudge to compensate for limited fixed point precision
  a = 256 - b;
  b++; // still testing fudge algorithm, possibly 'b = b > a ? b+1 : b;'

  // lo bits: mask, multiply, mask, shift
  res1 = (((pixel1 & 0x00FF00FF) * a) & 0xFF00FF00) >> 8;
  res1 += ((((pixel2 & 0x00FF00FF) * b) & 0xFF00FF00)) >> 8;

  // hi bits: mask, shift, multiply, mask
  res2 = (((pixel1 & 0xFF00FF00) >> 8) * a) & 0xFF00FF00;
  res2 += (((pixel2 & 0xFF00FF00) >> 8) * b) & 0xFF00FF00;

  return (res1 + res2) | 0xff000000; // set alpha = 255 if opaque
}

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

static inline SUCOMPLEX
pointProd(SUCOMPLEX a, SUCOMPLEX b)
{
  return a.real() * b.real() + SU_I * (a.imag() * b.imag());
}

static inline SUCOMPLEX
pointAbs(SUCOMPLEX a)
{
  return fabs(a.real()) + SU_I * fabs(a.imag());
}

WaveView::WaveView()
{
  this->waveTree = &this->ownWaveTree;
  this->borrowTree(*this);
}

void
WaveView::borrowTree(WaveView &view)
{
  if (this->waveTree != nullptr) {
    disconnect(
          this->waveTree,
          SIGNAL(ready(void)),
          this,
          nullptr);

    disconnect(
          this->waveTree,
          SIGNAL(progress(quint64, quint64)),
          this,
          nullptr);
  }

  this->waveTree = view.waveTree;
  connect(
        this->waveTree,
        SIGNAL(ready(void)),
        this,
        SLOT(onReady(void)));

  connect(
        this->waveTree,
        SIGNAL(progress(quint64, quint64)),
        this,
        SLOT(onProgress(quint64, quint64)));
}

qreal
WaveView::getEnvelope(void) const
{
  if (!this->waveTree->isComplete())
    return 0;

  if (this->waveTree->size() == 0)
    return 0;

  return SCAST(qreal, (*this->waveTree)[this->waveTree->size() - 1][0].envelope);
}

void
WaveView::setSampleRate(qreal rate)
{
  this->deltaT     = 1. / rate;
  this->sampleRate = rate;

  // Set horizontal zoom again
  this->setHorizontalZoom(this->start, this->end);
}

void
WaveView::setTimeStart(qreal t0)
{
  this->t0 = t0;
}

void
WaveView::setHorizontalZoom(qint64 start, qint64 end)
{
  this->start = start;
  this->end   = end;
  this->setGeometry(this->width, this->height);
}

void
WaveView::setVerticalZoom(qreal min, qreal max)
{
  this->min = min;
  this->max = max;
  this->setGeometry(this->width, this->height);
}

void
WaveView::setGeometry(int width, int height)
{
  this->width  = width;
  this->height = height;

  this->sampPerPx  = SCAST(qreal, this->end - this->start) / this->width;
  this->unitsPerPx = SCAST(qreal, this->max - this->min) / this->height;
}

/////////////////////////////// Drawing methods ////////////////////////////////
void
WaveView::drawWaveClose(QPainter &p)
{
  qreal firstSamp, lastSamp;
  qint64 firstIntegerSamp, lastIntegerSamp;
  SUSCOUNT length = this->waveTree->getLength();
  const SUCOMPLEX *data = this->waveTree->getData();
  int prevMinEnvY = 0;
  int prevMaxEnvY = 0;
  int nextX, currX, currY;
  int prevX = 0, prevY = 0;
  int pathX = 0;

  qreal prevPhase = 0;
  qreal alpha = 1;
  bool havePrevEnv = false;
  bool havePrevWf  = false;
  QColor color = this->foreground;
  QColor darkenedForeground;
  int minEnvY = 0, maxEnvY = 0;
  QPen pen;
  bool paintSamples = this->sampPerPx < 1. / (2 * WAVEFORM_CIRCLE_DIM);

  if (this->sampPerPx > 1)
    alpha = sqrt(1. / this->sampPerPx);

  pen.setColor(color);
  pen.setStyle(Qt::SolidLine);
  p.setPen(pen);

  // Initialize darkened foreground
  if (this->showWaveform && !this->showPhase) {
    darkenedForeground.setRed(this->foreground.red() / 3);
    darkenedForeground.setGreen(this->foreground.green() / 3);
    darkenedForeground.setBlue(this->foreground.blue() / 3);
  }

  // Determine the representation range
  firstSamp = this->px2samp(this->leftMargin);
  lastSamp  = this->px2samp(this->width - 1);

  firstIntegerSamp = SCAST(qint64, std::ceil(firstSamp));
  lastIntegerSamp  = SCAST(qint64, std::floor(lastSamp));

  if (firstIntegerSamp < 0)
    firstIntegerSamp = 0;

  if (lastIntegerSamp >= SCAST(qint64, length))
    lastIntegerSamp = SCAST(qint64, length - 1);

  nextX = SCAST(int, this->samp2px(SCAST(qreal, firstIntegerSamp)));
  for (qint64 i = firstIntegerSamp; i <= lastIntegerSamp; ++i) {
    currX = nextX;
    nextX = SCAST(int, this->samp2px(SCAST(qreal, i + 1)));
    currY = SCAST(int, this->value2px(this->cast(data[i])));

    if (i >= 0 && i < SCAST(qint64, length)) {
      // Draw envelope?
      if (this->showEnvelope) {
        // Determine limits
        qreal mag    = SCAST(qreal, SU_C_ABS(data[i]));
        qreal phase  = SCAST(qreal, SU_C_ARG(data[i]));

        int pxLower  = SCAST(int, this->value2px(+mag));
        int pxUpper  = SCAST(int, this->value2px(-mag));

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
          p.setOpacity(this->showWaveform ? .33 : 1.);

          if (havePrevEnv) {
            QPainterPath path;

            if (pathX != currX) {
              path.moveTo(pathX, prevMinEnvY);
              path.lineTo(currX, minEnvY);
              path.lineTo(currX, maxEnvY);
              path.lineTo(pathX, prevMaxEnvY);
            }

            // Show phase?
            if (this->showPhase) {
              if (this->showPhaseDiff) {
                // Display its first derivative (frequency)
                qreal phaseDiff = phase - prevPhase;
                if (phaseDiff < 0)
                  phaseDiff += 2. * SCAST(qreal, PI);
                QColor diffColor = this->phaseDiff2Color(phaseDiff);

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
                p.fillPath(path, this->foreground);
              } else {
                p.setPen(this->foreground);
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

      if (this->showWaveform) {
        p.setOpacity(alpha);

        if (havePrevWf) {
          p.setPen(QPen(this->foreground));
          p.drawLine(prevX, prevY, currX, currY);
        }

        if (paintSamples) {
          p.setBrush(QBrush(this->foreground));
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
  WaveViewTree::iterator view = this->waveTree->begin() + level;

  bits = (level + 1) * WAVEFORM_BLOCK_BITS;

  pen.setColor(this->foreground);
  pen.setStyle(Qt::SolidLine);
  p.setPen(pen);

  // Initialize darkened foreground

  // Determine the representation range
  firstSamp = this->px2samp(this->leftMargin);
  lastSamp  = this->px2samp(this->width - 1);

  firstBlock = SCAST(qint64, std::ceil(firstSamp)) >> bits;
  lastBlock  = SCAST(qint64, std::floor(lastSamp)) >> bits;

  if (firstBlock < 0)
    firstBlock = 0;

  if (lastBlock >= SCAST(qint64, view->size()))
    lastBlock = SCAST(qint64, view->size() - 1);

  nextX = SCAST(int, this->samp2px(SCAST(qreal, firstBlock << bits)));

  for (qint64 i = firstBlock; i <= lastBlock; ++i) {
    WaveLimits &z = (*view)[SCAST(unsigned long, i)];
    qint64 samp = i << bits;

    currX = nextX;
    nextX = SCAST(int, this->samp2px(SCAST(qreal, samp + (1 << bits))));

    // Draw envelope?
    if (this->showEnvelope) {
      // Determine limits
      qreal mag   = SCAST(qreal, z.envelope);
      qreal phase = SCAST(qreal, SU_C_ARG(z.mean));

      int pxHigh  = SCAST(int, this->value2px(+mag));
      int pxLow   = SCAST(int, this->value2px(-mag));

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
        p.setOpacity(this->showWaveform ? .33 : 1.);

        if (havePrev) {
          // Show phase?
          QColor lineColor;

          if (this->showPhase) {
            // Display its first derivative (frequency, cached)
            if (this->showPhaseDiff) {
              lineColor = this->phaseDiff2Color(
                    SCAST(qreal, z.freq < 0 ? z.freq + 2 * PI : z.freq));
            }
            else
              lineColor = phaseToColor(phase);
          } else {
            lineColor = this->foreground;
          }

          p.setPen(QPen(lineColor));
          p.drawLine(currX, minEnvY, currX, maxEnvY);
        }
      }
    }

    // Draw waveform
    if (this->showWaveform) {
      qreal min = this->cast(z.min);
      qreal max = this->cast(z.max);

      int yA = SCAST(int, this->value2px(min));
      int yB = SCAST(int, this->value2px(max));

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
        p.setOpacity(this->showEnvelope ? .33 : .66);
        p.setPen(QPen(this->foreground));
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
  this->setGeometry(painter.device()->width(), painter.device()->height());

  if (!this->waveTree->isComplete()) {
    QFont font;
    QFontMetrics metrics(font);
    QString text;
    QRect rect;
    int tw;

    if (this->waveTree->isRunning()) {
      if (this->lastProgressMax > 0)
        text = QString::asprintf(
              "Processing waveform (%ld%% complete)",
              100 * this->lastProgressCurr / this->lastProgressMax);
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
          this->width / 2 - tw / 2,
          this->height / 2 - metrics.height() / 2,
          tw,
          metrics.height());

    painter.setPen(this->foreground);
    painter.setOpacity(1);
    painter.drawText(rect, Qt::AlignHCenter | Qt::AlignBottom, text);

    return;
  }

  // Nothing to paint? Leave.
  if (this->waveTree->getLength() == 0
      || this->waveTree->size() == 0)
    return;

  if (this->sampPerPx > 8.) {
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
          floor(log(this->sampPerPx) / log(WAVEFORM_BLOCK_LENGTH))) - 1;
    if (level >= this->waveTree->size())
      level = this->waveTree->size() - 1;

    this->drawWaveFar(painter, level);
  } else {
    this->drawWaveClose(painter);
  }
}

void
WaveView::setBuffer(const std::vector<SUCOMPLEX> *buf)
{
  if (this->waveTree == &this->ownWaveTree) {
    this->waveTree->clear();
    this->waveTree->reprocess(buf->data(), buf->size());
  }
}

void
WaveView::refreshBuffer(const std::vector<SUCOMPLEX> *buf)
{
  if (this->waveTree == &this->ownWaveTree)
    this->waveTree->reprocess(buf->data(), buf->size());
}

///////////////////////////////////// Slots ////////////////////////////////////
void
WaveView::onReady(void)
{
  this->lastProgressCurr = this->lastProgressMax = 0;

  emit ready();
}

void
WaveView::onProgress(quint64 curr, quint64 max)
{
  this->lastProgressCurr = curr;
  this->lastProgressMax  = max;

  emit progress();
}

