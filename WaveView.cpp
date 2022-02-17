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
  return a.real() * b.real() + I * (a.imag() * b.imag());
}

static inline SUCOMPLEX
pointAbs(SUCOMPLEX a)
{
  return fabs(a.real()) + I * fabs(a.imag());
}

WaveView::WaveView()
{
  this->waveTree = &this->ownWaveTree;
}

void
WaveView::borrowTree(WaveView &view)
{
  this->waveTree = view.waveTree;
}

qreal
WaveView::getEnvelope(void) const
{
  if (this->waveTree->size() == 0)
    return 0;

  return SCAST(qreal, (*this->waveTree)[this->waveTree->size() - 1][0].envelope);
}

void
WaveView::setTimeUnits(qreal t0, qreal rate)
{
  this->t0         = t0;
  this->deltaT     = 1. / rate;
  this->sampleRate = rate;

  // Set horizontal zoom again
  this->setHorizontalZoom(this->start, this->end);
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

// Algorithm is as follows:
// 1. Traverse *p starting from `since` and compute min, max, mean and std
// 2. If next to p has less than the required elements, append
// 3. Continue until next p has size 1

void
WaveView::buildNextView(WaveViewTree::iterator p, SUSCOUNT since)
{
  WaveViewTree::iterator next = p + 1;
  SUSCOUNT length, nextLength;

  since >>= WAVEFORM_BLOCK_BITS;
  since <<= WAVEFORM_BLOCK_BITS;

  if (next == this->waveTree->end()) {
    this->waveTree->append(WaveLimitVector());
    next = this->waveTree->end() - 1;
    p    = next - 1;
    next->resize(1);
  }

  length = p->size();
  nextLength = (length + WAVEFORM_BLOCK_LENGTH - 1) >> WAVEFORM_BLOCK_BITS;

  if (next->size() < nextLength)
    next->resize(nextLength);

  for (auto i = since; i < length; i += WAVEFORM_BLOCK_LENGTH) {
    const WaveLimits *data = p->data() + i;
    WaveLimits thisLimit;
    quint64 left       = MIN(length - i, WAVEFORM_BLOCK_LENGTH);
    SUFLOAT kInv       = 1.f/ SU_ASFLOAT(left);

    thisLimit.max      = data[0].max;
    thisLimit.min      = data[0].min;

    for (SUSCOUNT j = 0; j < left; ++j) {
      if (data[j].max.real() > thisLimit.max.real())
        thisLimit.max = data[j].max.real() + thisLimit.max.imag() * I;
      if (data[j].max.imag() > thisLimit.max.imag())
        thisLimit.max = thisLimit.max.real() + data[j].max.imag() * I;

      if (data[j].min.real() < thisLimit.min.real())
        thisLimit.min = data[j].min.real() + thisLimit.min.imag() * I;
      if (data[j].min.imag() < thisLimit.min.imag())
        thisLimit.min = thisLimit.min.real() + data[j].min.imag() * I;

      if (thisLimit.envelope < data[j].envelope)
        thisLimit.envelope = data[j].envelope;

      thisLimit.mean += data[j].mean;
      thisLimit.freq += data[j].freq;
    }

    // Compute mean, mean frequency and finish
    thisLimit.mean *= kInv;
    thisLimit.freq *= kInv;

    (*next)[i >> WAVEFORM_BLOCK_BITS] = thisLimit;
  }

  if (next->size() > 1)
    buildNextView(next, since >> WAVEFORM_BLOCK_BITS);
}

void
WaveView::flush(void)
{
  if (this->waveTree == &this->ownWaveTree)
    this->waveTree->clear();

  this->data = nullptr;
  this->length = 0;
}

// Algorithm is as follows:
// 1. Construct waveTree[0] traversing data
// 2. Call buildNextView on waveTree[0] since 0
void
WaveView::build(const SUCOMPLEX *data, SUSCOUNT length, SUSCOUNT since)
{
  WaveViewTree::iterator next = this->waveTree->begin();
  SUSCOUNT nextLength;

  since >>= WAVEFORM_BLOCK_BITS;
  since <<= WAVEFORM_BLOCK_BITS;

  this->data   = data;
  this->length = length;

  // And that's it, that's the only thing we do if the tree is not ours.
  if (this->waveTree != &this->ownWaveTree)
    return;

  if (next == this->waveTree->end()) {
    this->waveTree->append(WaveLimitVector());
    next = this->waveTree->begin();
    next->resize(1);
  }

  nextLength = (length + WAVEFORM_BLOCK_LENGTH - 1) >> WAVEFORM_BLOCK_BITS;

  if (next->size() < nextLength)
    next->resize(nextLength);

  for (SUSCOUNT i = since; i < length; i += WAVEFORM_BLOCK_LENGTH) {
    WaveLimits thisLimit;
    SUFLOAT env2  = 0;
    quint64 left  = MIN(length - i, WAVEFORM_BLOCK_LENGTH);
    SUFLOAT kInv  = 1.f/ SU_ASFLOAT(left);

    data          = this->data + i;
    thisLimit.min = data[0];
    thisLimit.max = data[0];

    for (SUSCOUNT j = 0; j < left; ++j) {
      if (data[j].real() > thisLimit.max.real())
        thisLimit.max = data[j].real() + thisLimit.max.imag() * I;
      if (data[j].imag() > thisLimit.max.imag())
        thisLimit.max = thisLimit.max.real() + data[j].imag() * I;

      if (data[j].real() < thisLimit.min.real())
        thisLimit.min = data[j].real() + thisLimit.min.imag() * I;
      if (data[j].imag() < thisLimit.min.imag())
        thisLimit.min = thisLimit.min.real() + data[j].imag() * I;

      env2 = SU_C_REAL(data[j] * SU_C_CONJ(data[j]));
      if (thisLimit.envelope < env2)
        thisLimit.envelope = env2;

      if ((i + j) > 0)
        thisLimit.freq += SU_C_ARG(data[j] * SU_C_CONJ(data[j - 1]));

      thisLimit.mean += data[j];
    }

    thisLimit.freq *= kInv;
    thisLimit.mean *= kInv;
    thisLimit.envelope = sqrt(thisLimit.envelope);

    (*next)[i >> WAVEFORM_BLOCK_BITS] = thisLimit;
  }

  if (next->size() > 1)
    buildNextView(next, since >> WAVEFORM_BLOCK_BITS);
}

/////////////////////////////// Drawing methods ////////////////////////////////
void
WaveView::drawWaveClose(QPainter &p)
{
  qreal firstSamp, lastSamp;
  qint64 firstIntegerSamp, lastIntegerSamp;
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

  if (lastIntegerSamp >= SCAST(qint64, this->length))
    lastIntegerSamp = SCAST(qint64, this->length - 1);

  nextX = SCAST(int, this->samp2px(SCAST(qreal, firstIntegerSamp)));
  for (qint64 i = firstIntegerSamp; i <= lastIntegerSamp; ++i) {
    currX = nextX;
    nextX = SCAST(int, this->samp2px(SCAST(qreal, i + 1)));
    currY = SCAST(int, this->value2px(this->cast(data[i])));

    if (i >= 0 && i < SCAST(qint64, this->length)) {
      // Draw envelope?
      if (this->showEnvelope) {
        // Determine limits
        qreal mag    = SCAST(qreal, SU_C_ABS(this->data[i]));
        qreal phase  = SCAST(qreal, SU_C_ARG(this->data[i]));

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
        p.setOpacity(this->showEnvelope ? .33 : 1.);
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

  // Nothing to paint? Leave.
  if (this->length == 0 || this->waveTree->size() == 0)
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
