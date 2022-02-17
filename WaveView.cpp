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
#include "YIQ.h"

// See https://www.linuxquestions.org/questions/blog/rainbowsally-615861/qt-fast-pixel-color-mixing-35589/

static inline QRgb
qMixRgb(QRgb pixel1, QRgb pixel2, unsigned alpha)
{
  int a, b, c;
  uint res1, res2;

  b = static_cast<int>(alpha);
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
    angle += 2 * PI;

  return yiqTable[
        qBound(
          0,
        static_cast<int>(SU_FLOOR(1024 * angle / (2 * PI))),
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
  this->start        = start;
  this->end          = end;
  this->viewInterval = static_cast<qreal>(end - start) * this->deltaT;
}

void
WaveView::setVerticalZoom(qreal min, qreal max)
{
  this->min = min;
  this->max = max;
}

void
WaveView::setGeometry(int width, int height)
{
  this->width  = width;
  this->height = height;

  this->sampPerPx  = static_cast<qreal>(this->end - this->start) / this->width;
  this->unitsPerPx = static_cast<qreal>(this->max - this->min) / this->height;
}

// Algorithm is as follows:
// 1. Traverse *p starting from `since` and compute min, max, mean and std
// 2. If next to p has less than the required elements, append
// 3. Continue until next p has size 1

void
WaveView::buildNextView(WaveViewList::iterator p, SUSCOUNT since)
{
  WaveViewList::iterator next = p + 1;
  SUSCOUNT length, nextLength;
  SUFLOAT kInv = 1.f / SU_ASFLOAT(WAVEFORM_BLOCK_LENGTH);

  since /= WAVEFORM_BLOCK_LENGTH;
  since *= WAVEFORM_BLOCK_LENGTH;

  if (next == this->views.end()) {
    this->views.append(WaveLimitVector());
    next = this->views.end() - 1;
    p    = next - 1;
    next->resize(1);
  }

  length = p->size();
  nextLength = (length + WAVEFORM_BLOCK_LENGTH - 1) / WAVEFORM_BLOCK_LENGTH;

  if (next->size() < nextLength)
    next->resize(nextLength);

  for (auto i = since; i < length; i += WAVEFORM_BLOCK_LENGTH) {
    const WaveLimits *data = p->data() + i;
    WaveLimits thisLimit;

    thisLimit.max      = data[0].max;
    thisLimit.min      = data[0].min;

    for (auto j = 0; j < WAVEFORM_BLOCK_LENGTH; ++j) {
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

    (*next)[i / WAVEFORM_BLOCK_LENGTH] = thisLimit;
  }

  if (next->size() > 1)
    buildNextView(next, since / WAVEFORM_BLOCK_LENGTH);
}

// Algorithm is as follows:
// 1. Construct views[0] traversing data
// 2. Call buildNextView on views[0] since 0


void
WaveView::build(const SUCOMPLEX *data, SUSCOUNT length, SUSCOUNT since)
{
  WaveViewList::iterator next = this->views.begin();
  SUSCOUNT nextLength;
  SUFLOAT kInv = 1.f / SU_ASFLOAT(WAVEFORM_BLOCK_LENGTH);
  struct timeval tv, otv, diff;

  gettimeofday(&otv, nullptr);

  since /= WAVEFORM_BLOCK_LENGTH;
  since *= WAVEFORM_BLOCK_LENGTH;

  this->data   = data;
  this->length = length;

  if (next == this->views.end()) {
    this->views.append(WaveLimitVector());
    next = this->views.begin();
    next->resize(1);
  }

  nextLength = (length + WAVEFORM_BLOCK_LENGTH - 1) / WAVEFORM_BLOCK_LENGTH;

  if (next->size() < nextLength)
    next->resize(nextLength);

  for (SUSCOUNT i = since; i < length; i += WAVEFORM_BLOCK_LENGTH) {
    WaveLimits thisLimit;
    SUFLOAT env2 = 0;

    thisLimit.min = data[0];
    thisLimit.max = data[0];
    data          = this->data + i;

    for (SUSCOUNT j = 0; j < WAVEFORM_BLOCK_LENGTH; ++j) {
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

    (*next)[i / WAVEFORM_BLOCK_LENGTH] = thisLimit;
  }

  if (next->size() > 1)
    buildNextView(next, since / WAVEFORM_BLOCK_LENGTH);

  gettimeofday(&tv, nullptr);
  timersub(&tv, &otv, &diff);
}

/////////////////////////////// Drawing methods ////////////////////////////////
void
WaveView::drawWaveClose(QPainter &p)
{
  qreal firstSamp, lastSamp;
  qint64 firstIntegerSamp, lastIntegerSamp;
  int prevPxHigh = 0;
  int prevPxLow = 0;
  int currX, currY;
  int prevX = 0, prevY = 0;

  qreal prevPhase = 0;
  qreal alpha = 1;
  bool havePrev = false;
  QColor color = this->foreground;
  QColor darkenedForeground;
  QPen pen;

  if (this->sampPerPx > 1)
    alpha = sqrt(this->sampPerPx);

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

  firstIntegerSamp = static_cast<qint64>(std::ceil(firstSamp));
  lastIntegerSamp  = static_cast<qint64>(std::floor(lastSamp));

  if (firstIntegerSamp < 0)
    firstIntegerSamp = 0;

  if (lastIntegerSamp >= this->length)
    lastIntegerSamp = this->length - 1;

  for (qint64 i = firstIntegerSamp; i <= lastIntegerSamp; ++i) {
    currX = static_cast<int>(this->samp2px(static_cast<qreal>(i)));
    currY = static_cast<int>(this->value2px(this->cast(data[i])));

    if (i >= 0 && i < this->length) {
      // Draw envelope?
      if (this->showEnvelope) {
        // Determine limits
        qreal mag   = static_cast<qreal>(SU_C_ABS(this->data[i]));
        qreal phase = static_cast<qreal>(SU_C_ARG(this->data[i]));

        int pxHigh = static_cast<int>(this->value2px(+mag));
        int pxLow  = static_cast<int>(this->value2px(-mag));

        p.setPen(Qt::NoPen);
        p.setOpacity(this->showWaveform ? .33 : 1.);

        if (havePrev) {
          QPainterPath path;
          path.moveTo(prevX, prevPxHigh);
          path.lineTo(currX, pxHigh);
          path.lineTo(currX, pxLow);
          path.lineTo(prevX, prevPxLow);

          // Show phase?
          if (this->showPhase) {
            if (this->showPhaseDiff) {
              // Display its first derivative (frequency)
              qreal phaseDiff = phase - prevPhase;
              if (phaseDiff < 0)
                phaseDiff += 2. * static_cast<qreal>(PI);
              QColor diff = this->phaseDiff2Color(phaseDiff);
              p.fillPath(path, diff);
            } else {
              // Display it as-is
              QLinearGradient gradient(prevX, 0, currX, 0);
              gradient.setColorAt(0, phaseToColor(prevPhase));
              gradient.setColorAt(1, phaseToColor(phase));
              p.fillPath(path, gradient);
            }
          } else {
            p.fillPath(path, this->foreground);
          }
        }
        prevPxHigh = pxHigh;
        prevPxLow  = pxLow;
        prevPhase  = phase;
      }

      if (this->showWaveform && havePrev) {
        p.setOpacity(alpha);
        p.setPen(QPen(this->foreground));
        p.drawLine(prevX, prevY, currX, currY);
      }
    }

    prevX = currX;
    prevY = currY;
    havePrev = true;
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
  int currX, currY;
  int prevX = 0, prevY = 0;
  int bits;
  qreal prevPhase = 0;
  qreal blocksPerPx;
  bool havePrev = false;
  QColor color = this->foreground;
  QColor darkenedForeground;
  qreal alpha = 1.;
  QPen pen;
  WaveViewList::iterator view = this->views.begin() + level;

  bits = (level + 1) * WAVEFORM_BLOCK_BITS;

  blocksPerPx = this->sampPerPx / (1 << bits);

  //
  // k = 1 / blocksPerPx
  // exp(-(blocksPerPx - 1))
  //
  if (blocksPerPx > 1) {
    alpha = exp(-(blocksPerPx - 1) / (WAVEFORM_BLOCK_LENGTH - 1) * 1.25);
    if (alpha < 1./ WAVEFORM_BLOCK_LENGTH)
      alpha = 1. / WAVEFORM_BLOCK_LENGTH;
  }

  pen.setColor(color);
  pen.setStyle(Qt::SolidLine);
  p.setPen(pen);

  // Initialize darkened foreground

  // Determine the representation range
  firstSamp = this->px2samp(this->leftMargin);
  lastSamp  = this->px2samp(this->width - 1);

  firstBlock = static_cast<qint64>(std::ceil(firstSamp)) >> bits;
  lastBlock  = static_cast<qint64>(std::floor(lastSamp)) >> bits;

  if (firstBlock < 0)
    firstBlock = 0;

  if (lastBlock >= view->size())
    lastBlock = view->size() - 1;

  for (qint64 i = firstBlock; i <= lastBlock; ++i) {
    WaveLimits &z = (*view)[i];
    qint64 samp = i << bits;

    currX = static_cast<int>(this->samp2px(static_cast<qreal>(samp)));
    currY = static_cast<int>(this->value2px(this->cast(z.mean)));

    // Draw envelope?
    if (this->showEnvelope) {
      // Determine limits
      qreal mag   = z.envelope;
      qreal phase = static_cast<qreal>(SU_C_ARG(z.mean));

      int pxHigh = static_cast<int>(this->value2px(+mag));
      int pxLow  = static_cast<int>(this->value2px(-mag));

      p.setPen(Qt::NoPen);
      p.setOpacity(this->showWaveform ? .33 : 1.);

      if (havePrev) {
        // Show phase?
        QColor lineColor;

        if (this->showPhase) {
          // Display its first derivative (frequency, cached)
          if (this->showPhaseDiff) {
            lineColor = this->phaseDiff2Color(
                  z.freq < 0
                  ? z.freq + 2 * PI
                  : z.freq);
          }
          else
            lineColor = phaseToColor(phase);
        } else {
          lineColor = this->foreground;
        }

        p.setPen(QPen(lineColor));
        p.drawLine(currX, pxHigh, currX, pxLow);
      }

      prevPhase  = phase;
    }

    if (this->showWaveform) {
      qreal min = this->cast(z.min);
      qreal max = this->cast(z.max);

      int yA = static_cast<int>(this->value2px(min));
      int yB = static_cast<int>(this->value2px(max));

      // These comparisons seem inverted. They are not: remember that
      // vertical screen coordinates are top-down, while regular cartesian
      // coordinates are bottom-up. This results in a change of signedness
      // in the vertical coordinate.

      if (havePrev) {
        if (yA < prevY)
          yA = prevY;
        if (yB > prevY)
          yB = prevY;
      }

      p.setOpacity(alpha);
      p.setPen(QPen(this->foreground));
      p.drawLine(currX, yA, currX, yB);
    }

    prevX = currX;
    prevY = currY;
    havePrev = true;
  }
}

void
WaveView::drawWave(QPainter &painter)
{
  this->setGeometry(painter.device()->width(), painter.device()->height());

  // Nothing to paint? Leave.
  if (this->length == 0)
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

    level = static_cast<int>(
          floor(log(this->sampPerPx) / log(WAVEFORM_BLOCK_LENGTH))) - 1;
    if (level >= this->views.size())
      level = this->views.size() - 1;

    this->drawWaveFar(painter, level);
  } else {
    this->drawWaveClose(painter);
  }
}
