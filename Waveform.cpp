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

#include "Waveform.h"
#include <QPainter>
#include <QColormap>
#include <SuWidgetsHelpers.h>
#include "YIQ.h"

// See https://www.linuxquestions.org/questions/blog/rainbowsally-615861/qt-fast-pixel-color-mixing-35589/

static QRgb
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

static QColor const &
phaseToColor(SUFLOAT angle)
{
  if (angle < 0)
    angle += 2 * PI;

  return yiqTable[
        qBound(
          0,
        static_cast<int>(SU_FLOOR(1024 * angle / (2 * PI))),
        1023)];
}

////////////////////////// WaveBuffer methods //////////////////////////////////
WaveBuffer::WaveBuffer()
{
  this->buffer = &this->ownBuffer;
}

WaveBuffer::WaveBuffer(const std::vector<SUCOMPLEX> *vec)
{
  this->loan = true;
  this->buffer = vec;
}

bool
WaveBuffer::feed(SUCOMPLEX val)
{
  if (this->loan)
    return false;

  this->ownBuffer.push_back(val);

  return true;
}

bool
WaveBuffer::feed(std::vector<SUCOMPLEX> const &vec)
{
  if (this->loan)
    return false;

  this->ownBuffer.insert(this->ownBuffer.end(), vec.begin(), vec.end());

  return true;
}

size_t
WaveBuffer::length(void) const
{
  return this->buffer->size();
}

const SUCOMPLEX *
WaveBuffer::data(void) const
{
  return this->buffer->data();
}

////////////////////////// Geometry methods ////////////////////////////////////
void
Waveform::recalculateDisplayData(void)
{
  qreal range;
  qreal divLen;

  this->sampPerPx = static_cast<qreal>(this->end - this->start)
      / this->geometry.width();
  this->unitsPerPx = static_cast<qreal>(this->max - this->min)
      / this->geometry.height();

  // In every direction must be a minimum of 5 divisions, and a maximum of 10
  range = (this->end - this->start) * deltaT;

  //
  // 7.14154 - 7.143. Range is 0.00146.
  // First significant digit: at 1e-3
  //  7.141 ... 2 ...  3. Only 3 divs if rounding to the 1e-3.
  //  7.141 ...15 ... 20 ... 25 ... 30. 5 divs if rounding to de 5e-4!

  divLen = pow(10, std::floor(std::log10(range)));

  // We progressively divide the division length, until we find a good match

  if (range / divLen < 5) {
    divLen /= 2;
    if (range / divLen < 5) {
      divLen /= 2.5;
      if (range / divLen < 5) {
        divLen /= 4;
      }
    }
  }

  this->hDivSamples = divLen * this->sampleRate;
  this->hDigits     = static_cast<int>(std::floor(std::log10(divLen)));



  // Same procedure for vertical axis
  range = this->max - this->min;
  divLen = pow(10, std::floor(std::log10(range)));
  if (range / divLen < 5) {
    divLen /= 2;
    if (range / divLen < 5) {
      divLen /= 2.5;
      if (range / divLen < 5) {
        divLen /= 4;
      }
    }
  }

  // Conversion not necessary.
  this->vDivUnits = divLen;
  this->vDigits   = static_cast<int>(std::floor(std::log10(divLen)));
}

void
Waveform::zoomHorizontalReset(void)
{
  if (this->haveGeometry) {
    if (this->data.length() <= static_cast<unsigned>(this->geometry.width()))
      this->zoomHorizontal(0, static_cast<qint64>(this->geometry.width() - 1));
    else
      this->zoomHorizontal(0, static_cast<qint64>(this->data.length()) - 1);
  }
}

void
Waveform::zoomHorizontal(qint64 x, qreal amount)
{
  qreal newRange;
  qreal fixedSamp;
  qreal relPoint = static_cast<qreal>(x) / this->geometry.width();

  //
  // This means that position at x remains the same, while the others shrink
  // or stretch accordingly
  //

  fixedSamp = std::round(this->px2samp(x));
  newRange = std::ceil(amount * (this->end - this->start));

  this->zoomHorizontal(
        static_cast<qint64>(std::floor(fixedSamp - relPoint * newRange)),
        static_cast<qint64>(std::ceil(fixedSamp + (1.0 - relPoint) * newRange)));
}

void
Waveform::zoomHorizontal(qreal tStart, qreal tEnd)
{
  this->zoomHorizontal(
        static_cast<qint64>(this->t2samp(tStart)),
        static_cast<qint64>(this->t2samp(tEnd)));
}

void
Waveform::zoomHorizontal(qint64 start, qint64 end)
{
  if (start != this->start || end != this->end) {
    this->start = start;
    this->end = end;
    if (this->hSelection)
      this->selectionDrawn = false;
    this->axesDrawn = false;
    this->recalculateDisplayData();
    emit horizontalRangeChanged(start, end);
  }
}

void
Waveform::saveHorizontal(void)
{
  this->savedStart = this->start;
  this->savedEnd   = this->end;
}

void
Waveform::scrollHorizontal(qint64 orig, qint64 to)
{
  this->scrollHorizontal(to - orig);
}

void
Waveform::scrollHorizontal(qint64 delta)
{
  this->zoomHorizontal(
        this->savedStart - static_cast<qint64>(delta * this->sampPerPx),
        this->savedEnd   - static_cast<qint64>(delta * this->sampPerPx));
}

void
Waveform::selectHorizontal(qreal orig, qreal to)
{
  this->hSelection = true;

  if (orig < to) {
    this->hSelStart = orig;
    this->hSelEnd   = to;
  } else if (to < orig) {
    this->hSelStart = to;
    this->hSelEnd   = orig;
  } else {
    this->hSelection = false;
  }

  this->selectionDrawn = false;

  emit horizontalSelectionChanged(this->hSelStart, this->hSelEnd);
}

bool
Waveform::getHorizontalSelectionPresent(void) const
{
  return this->hSelection;
}

qreal
Waveform::getHorizontalSelectionStart(void) const
{
  return this->hSelStart;
}

qreal
Waveform::getHorizontalSelectionEnd(void) const
{
  return this->hSelEnd;
}

void
Waveform::setAutoScroll(bool value)
{
  this->autoScroll = value;
  this->refreshData();
}

void
Waveform::zoomVerticalReset(void)
{
  this->zoomVertical(-1., 1.);
}

void
Waveform::zoomVertical(qint64 y, qreal amount)
{
  qreal val = this->px2value(y);

  this->zoomVertical(
        (this->min - val) * amount + val,
        (this->max - val) * amount + val);
}

void
Waveform::zoomVertical(qreal start, qreal end)
{
  this->min = start;
  this->max = end;
  this->axesDrawn = false;

  this->recalculateDisplayData();
  emit verticalRangeChanged(start, end);
}

void
Waveform::saveVertical(void)
{
  this->savedMin = this->min;
  this->savedMax = this->max;
}

void
Waveform::scrollVertical(qint64 orig, qint64 to)
{
  this->scrollVertical(to - orig);
}

void
Waveform::scrollVertical(qint64 delta)
{
  this->zoomVertical(
        this->savedMin + delta * this->unitsPerPx,
        this->max = this->savedMax + delta * this->unitsPerPx);
}

void
Waveform::selectVertical(qint64 orig, qint64 to)
{
  this->vSelection = true;

  if (orig < to) {
    this->vSelStart = orig;
    this->vSelEnd   = to;
  } else if (to < orig) {
    this->vSelStart = to;
    this->vSelEnd   = orig;
  } else {
    this->vSelection = false;
  }

  this->selectionDrawn = false;

  emit verticalSelectionChanged(this->vSelStart, this->vSelEnd);
}

bool
Waveform::getVerticalSelectionPresent(void) const
{
  return this->vSelection;
}

qreal
Waveform::getVerticalSelectionStart(void) const
{
  return this->vSelStart;
}

qreal
Waveform::getVerticalSelectionEnd(void) const
{
  return this->vSelEnd;
}

void
Waveform::fitToEnvelope(void)
{
  qreal min = +std::numeric_limits<qreal>::infinity();
  qreal max = -std::numeric_limits<qreal>::infinity();
  const SUCOMPLEX *data = this->data.data();
  size_t length = this->data.length();

  for (unsigned i = 0; i < length; ++i) {
    if (cast(data[i]) > max)
      max = cast(data[i]);
    if (cast(data[i]) < min)
      min = cast(data[i]);
  }

  if (max > min)
    this->zoomVertical(min, max);
}

void
Waveform::setAutoFitToEnvelope(bool autoFit)
{
  this->autoFitToEnvelope = autoFit;
}


void
Waveform::resetSelection(void)
{
  this->hSelection = this->vSelection = false;
  this->selectionDrawn = false;
}

void
Waveform::setPeriodicSelection(bool val)
{
  this->periodicSelection = val;
  this->selectionDrawn = false;
}

///////////////////////////////// Events ///////////////////////////////////////
void
Waveform::mouseMoveEvent(QMouseEvent *event)
{
  // Emit mouseMove
  this->haveCursor = true;
  this->currMouseX = event->x();

  if (this->frequencyDragging)
    this->scrollHorizontal(this->clickX, event->x());
  else if (this->valueDragging)
    this->scrollVertical(this->clickY, event->y());
  else if (this->hSelDragging)
    this->selectHorizontal(
          static_cast<qint64>(this->px2samp(this->clickX)),
          static_cast<qint64>(this->px2samp(event->x())));

  emit hoverTime(this->px2t(this->currMouseX));
  this->invalidate();
}

void
Waveform::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::RightButton) {
    this->zoomHorizontalReset();
    this->invalidateHard();
  } else {
    // Save selection
    this->saveHorizontal();
    this->saveVertical();

    this->clickX = event->x();
    this->clickY = event->y();

    if (event->button() == Qt::MidButton
        || this->clickY >= this->geometry.height() - this->frequencyTextHeight)
      this->frequencyDragging = true;
    else if (this->clickX < this->valueTextWidth)
      this->valueDragging = true;
    else
      this->hSelDragging = true;
  }
}

void
Waveform::mouseReleaseEvent(QMouseEvent *event)
{
  this->mouseMoveEvent(event);
  this->frequencyDragging = false;
  this->valueDragging     = false;
  this->hSelDragging      = false;
}

void
Waveform::wheelEvent(QWheelEvent *event)
{
  qreal amount = std::pow(
        static_cast<qreal>(1.1),
        static_cast<qreal>(-event->delta() / 120.));
  if (event->x() < this->valueTextWidth)
    this->zoomVertical(static_cast<qint64>(event->y()), amount);
  else
    this->zoomHorizontal(static_cast<qint64>(event->x()), amount);

  this->invalidate();
}

void
Waveform::leaveEvent(QEvent *)
{
  this->haveCursor = false;
  this->invalidate();
}

//////////////////////////////// Drawing methods ///////////////////////////////////
SUCOMPLEX
Waveform::getMagPhase(qint64 sample)
{
  SUCOMPLEX val;

  if (sample < 0 || sample >= static_cast<qint64>(this->getDataLength()))
    return 0;

  if (!this->haveMagPhaseInfo) {
    this->magPhase =
          std::vector<SUCOMPLEX>(
            this->getDataLength(), std::numeric_limits<SUFLOAT>::quiet_NaN());
    this->haveMagPhaseInfo = true;
  }

  val = this->magPhase[static_cast<quint64>(sample)];

  if (std::isnan(SU_C_REAL(val)) || std::isnan(SU_C_IMAG(val))) {
    val =
        SU_C_ABS(this->data.data()[sample])
        + I * SU_C_ARG(this->data.data()[sample]);

    this->magPhase[static_cast<quint64>(sample)] = val;
  }

  return val;
}

void
Waveform::drawSelection(void)
{
  QPainter p(&this->selectionPixmap);

  p.fillRect(
        0,
        0,
        this->geometry.width(),
        this->geometry.height(),
        this->background);

  if (this->hSelection) {
    qreal selLen = this->hSelEnd - this->hSelStart;
    QRect rect(
          static_cast<int>(this->samp2px(this->hSelStart)),
          0,
          static_cast<int>(selLen / this->sampPerPx),
          this->geometry.height());

    if (rect.x() < this->valueTextWidth)
      rect.setX(this->valueTextWidth);

    if (rect.right() >= this->geometry.width())
      rect.setRight(this->geometry.width() - 1);


    p.fillRect(rect, this->selection);

    p.end();
  }
}

void
Waveform::drawWave(void)
{
  const SUCOMPLEX *data = this->data.data();
  int length = static_cast<int>(this->data.length());
  bool havePrev = false;
  QColor darkenedForeground;

  if (this->showWaveform && !this->showPhase) {
    darkenedForeground.setRed(this->foreground.red() / 3);
    darkenedForeground.setGreen(this->foreground.green() / 3);
    darkenedForeground.setBlue(this->foreground.blue() / 3);
  }

  waveform.fill(Qt::transparent);

  if (this->sampPerPx > 1) {
    std::vector<int> history(static_cast<size_t>(this->geometry.height()));
    int iters = static_cast<int>(std::floor(this->sampPerPx));
    int skip = 1;
    int count = 0;
    int samp = 0;
    int y = 0;
    int pxHigh, pxLow;
    int prev_y;
    QPainter p(&this->waveform);

    p.setPen(this->foreground);

    if (iters > WAVEFORM_MAX_ITERS)
      skip = iters / WAVEFORM_MAX_ITERS;

    for (int i = this->valueTextWidth; i < this->geometry.width(); ++i) {
      std::fill(history.begin(), history.end(), 0);
      samp = static_cast<int>(std::floor(this->px2samp(i)));

      count = 0;
      if (samp >= 0 && samp < length - iters) {
        int hMin = this->geometry.height();
        int hMax = 0;
        SUCOMPLEX mp;
        SUFLOAT mag = 0;
        int redAcc = 0, greenAcc = 0, blueAcc = 0;
        SUFLOAT phase, phaseDiff = 0;
        SUFLOAT absMax = 0;

        // Compose history
        for (int j = 0; j < iters; j += skip) {
          ++count;
          mp = this->getMagPhase(samp + j);

          mag = SU_C_REAL(mp);
          phase = SU_C_IMAG(mp);
          phaseDiff = phase - SU_C_IMAG(this->getMagPhase(samp + j - 1));

          if (mag > absMax)
            absMax = mag;

          if (phaseDiff < 0)
            phaseDiff += 2 * PI;

          const QColor &color =
              this->showPhaseDiff
              ? this->phaseDiff2Color(phaseDiff)
              : phaseToColor(phase);

          redAcc   += color.red();
          greenAcc += color.green();
          blueAcc  += color.blue();

          y = static_cast<int>(this->value2px(this->cast(data[samp + j])));

          if (havePrev) {
            for (int k = std::min(y, prev_y); k < std::max(y, prev_y); ++k)
              if (k >= 0 && k < this->geometry.height()) {
                ++history[static_cast<unsigned>(k)];
                if (k > hMax)
                  hMax = k;
                if (k < hMin)
                  hMin = k;
              }
          }

          havePrev = true;
          prev_y = y;

          if (j + skip > iters && j != iters - 1)
            j = iters - skip - 1;
        }

        if (this->showEnvelope) {
          // If draw envelope
          if (this->showPhase) {
            if (this->showWaveform)
              p.setPen(QColor(redAcc / count >> 1, greenAcc / count >> 1, blueAcc / count >> 1));
            else
              p.setPen(QColor(redAcc / count, greenAcc / count, blueAcc / count));
          } else {
            if (this->showWaveform)
              p.setPen(darkenedForeground);
          }

          pxHigh = static_cast<int>(this->value2px(static_cast<qreal>(absMax)));
          pxLow  = static_cast<int>(this->value2px(-static_cast<qreal>(absMax)));

          p.drawLine(i, pxLow, i, pxHigh);
        }

        if (this->showWaveform) {
          // Draw it
          unsigned int color =
              0xffffff & QColormap::instance().pixel(this->foreground);
          unsigned int alpha;

          for (int j = hMin; j <= hMax; ++j) {
            alpha = 63 + (192 * history[static_cast<unsigned>(j)]) / count;

            if (this->showEnvelope) {
              // Blend
              QRgb prev = waveform.pixel(i, j);
              waveform.setPixel(
                    i,
                    j,
                    qMixRgb(prev, color, alpha));
            } else {
              waveform.setPixel(
                    i,
                    j,
                    (alpha << 24) | color);
            }
          }
        }
      }
    }
  } else {
    qreal firstSamp, lastSamp;
    int firstIntegerSamp, lastIntegerSamp;
    int prevPxHigh = 0;
    int prevPxLow = 0;
    int prevX = 0, currX;
    SUCOMPLEX prevMp = 0;
    QPainter p(&this->waveform);
    QPen pen(this->foreground);
    // Two cases: if sampPerPx > 1: create small history of samples. Otherwise,
    // just interpolate

    pen.setStyle(Qt::SolidLine);
    p.setPen(pen);

    firstSamp = this->px2samp(this->valueTextWidth);
    lastSamp  = this->px2samp(this->geometry.width() - 1);

    firstIntegerSamp = static_cast<int>(std::ceil(firstSamp));
    lastIntegerSamp  = static_cast<int>(std::floor(lastSamp));

    for (int i = firstIntegerSamp; i <= lastIntegerSamp; ++i) {
      currX = static_cast<int>(this->samp2px(i));

      if (i >= 0 && i < length) {
        if (this->showEnvelope) {
          SUCOMPLEX mp = this->getMagPhase(i);
          int pxHigh = static_cast<int>(
                this->value2px(static_cast<qreal>(SU_C_REAL(mp))));

          int pxLow = static_cast<int>(
                this->value2px(-static_cast<qreal>(SU_C_REAL(mp))));

          p.setPen(Qt::NoPen);
          QPainterPath path;

          if (havePrev) {
            path.moveTo(prevX, prevPxHigh);
            path.lineTo(currX, pxHigh);
            path.lineTo(currX, pxLow);
            path.lineTo(prevX, prevPxLow);

            if (this->showPhase) {
              if (this->showPhaseDiff) {
                SUFLOAT phaseDiff = SU_C_IMAG(mp) - SU_C_IMAG(prevMp);

                if (phaseDiff < 0)
                  phaseDiff += 2 * PI;

                QColor diff = this->phaseDiff2Color(phaseDiff);
                if (this->showWaveform)
                  p.setOpacity(0.5);
                p.fillPath(path, diff);
              } else {
                QLinearGradient gradient(prevX, 0, currX, 0);
                gradient.setColorAt(0, phaseToColor(SU_C_IMAG(prevMp)));
                gradient.setColorAt(1, phaseToColor(SU_C_IMAG(mp)));
                if (this->showWaveform)
                  p.setOpacity(0.5);
                p.fillPath(path, gradient);
              }
            } else {
              p.fillPath(
                    path,
                    this->showWaveform ? darkenedForeground : this->foreground);
            }
          }
          prevPxHigh = pxHigh;
          prevPxLow = pxLow;
          prevMp = mp;
        }

        if (this->showWaveform && havePrev) {
          p.setOpacity(1);
          p.setPen(this->foreground);
          p.drawLine(
                prevX,
                static_cast<int>(this->value2px(this->cast(data[i - 1]))),
                currX,
                static_cast<int>(this->value2px(this->cast(data[i]))));
        }
      }

      prevX = currX;
      havePrev = true;
    }

    p.end();
  }
}

void
Waveform::drawVerticalAxes(void)
{
  QFont font;
  QPainter p(&this->axesPixmap);
  QFontMetrics metrics(font);
  QRect rect;
  QPen pen(this->axes);
  int axis;
  int px;

  pen.setStyle(Qt::DotLine);
  p.setPen(pen);
  p.setFont(font);

  this->frequencyTextHeight = metrics.height();

  if (this->hDivSamples > 0) {
    // Draw axes
    axis = static_cast<int>(std::floor(this->start / this->hDivSamples));

    while (axis * this->hDivSamples <= this->end) {
      px = static_cast<int>(this->samp2px(axis * this->hDivSamples));

      if (px > 0)
        p.drawLine(px, 0, px, this->geometry.height() - 1);
      ++axis;
    }

    // Draw labels
    p.setPen(this->text);
    axis = static_cast<int>(std::floor(this->start / this->hDivSamples));
    while (axis * this->hDivSamples <= this->end) {
      px = static_cast<int>(this->samp2px(axis * this->hDivSamples));

      if (px > 0) {
        QString label;
        int tw;

        label = SuWidgetsHelpers::formatQuantity(
              axis * this->hDivSamples * this->deltaT,
              this->hDigits);

        tw = metrics.width(label);

        rect.setRect(
              px - tw / 2,
              this->geometry.height() - this->frequencyTextHeight,
              tw,
              this->frequencyTextHeight);
        p.drawText(rect, Qt::AlignHCenter | Qt::AlignBottom, label);
      }
      ++axis;
    }
  }
  p.end();
}

void
Waveform::drawHorizontalAxes(void)
{
  QFont font;
  QPainter p(&this->axesPixmap);
  QFontMetrics metrics(font);
  QRect rect;
  QPen pen(this->axes);
  int axis;
  int px;

  p.setPen(pen);
  p.setFont(font);

  this->valueTextWidth = 0;

  if (this->vDivUnits > 0) {
    axis = static_cast<int>(std::floor(this->min / this->vDivUnits));

    while (axis * this->vDivUnits <= this->max) {
      pen.setStyle(axis == 0 ? Qt::SolidLine : Qt::DotLine);
      p.setPen(pen);
      px = static_cast<int>(this->value2px(axis * this->vDivUnits));

      if (px > 0)
        p.drawLine(0, px, this->geometry.width() - 1, px);
      ++axis;
    }

    p.setPen(this->text);
    axis = static_cast<int>(std::floor(this->min / this->vDivUnits));
    while (axis * this->vDivUnits <= this->max) {
      px = static_cast<int>(this->value2px(axis * this->vDivUnits));

      if (px > 0) {
        QString label;
        int tw;

        label = SuWidgetsHelpers::formatQuantity(
              axis * this->vDivUnits,
              this->vDigits,
              "");

        tw = metrics.width(label);

        rect.setRect(
              0,
              px - metrics.height() / 2,
              tw,
              metrics.height());

        if (tw > this->valueTextWidth)
          this->valueTextWidth = tw;

        p.fillRect(rect, this->background);
        p.drawText(rect, Qt::AlignHCenter | Qt::AlignBottom, label);
      }
      ++axis;
    }
  }

  p.end();
}

void
Waveform::drawAxes(void)
{
  this->axesPixmap.fill(Qt::transparent);
  this->drawHorizontalAxes();
  this->drawVerticalAxes();
}

void
Waveform::overlaySelectionMarkes(QPainter &p)
{
  if (this->periodicSelection) {
    qreal selLen = this->hSelEnd - this->hSelStart;
    // The sample buffer is divided in pieces of selLen samples,
    // starting from selLen.
    qint64 firstDiv = static_cast<int>((this->start - this->hSelStart) / selLen);
    qint64 lastDiv  = static_cast<int>((this->end - this->hSelStart) / selLen);

    qreal deltaDiv = selLen / static_cast<qreal>(this->divsPerSelection);

    p.setOpacity(.5);

    if (this->divsPerSelection == 1)
      p.setPen(this->axes);

    if (this->divsPerSelection * (lastDiv - firstDiv)
        >= this->geometry.width()) {
      for (qint64 i = this->valueTextWidth; i < this->geometry.width(); ++i) {
        qreal divSample = this->px2samp(i);
        p.setPen(divSample >= this->hSelStart && divSample <= this->hSelEnd
                 ? this->subSelection
                 : this->axes);
        p.drawLine(
              static_cast<int>(i),
              0,
              static_cast<int>(i),
              this->geometry.height() - this->frequencyTextHeight);
      }
    } else {
      for (qint64 i = firstDiv - 1; i <= lastDiv; ++i) {
        qreal sample = i * selLen + this->hSelStart;
        p.setOpacity(1);
        if (this->divsPerSelection > 1) {
          for (int j = 0; j <= this->divsPerSelection; ++j) {
            qreal divSample = sample + j * deltaDiv;
            int px = static_cast<int>(this->samp2px(divSample));

            p.setPen(divSample >= this->hSelStart && divSample <= this->hSelEnd
                     ? this->subSelection
                     : this->axes);

            if (px > this->valueTextWidth && px < this->geometry.width())
              p.drawLine(
                    px,
                    0,
                    px,
                    this->geometry.height() - this->frequencyTextHeight);
          }
        } else {
          int px = static_cast<int>(this->samp2px(sample));
          if (px > this->valueTextWidth && px < this->geometry.width())
            p.drawLine(
                  px,
                  0,
                  px,
                  this->geometry.height() - this->frequencyTextHeight);
        }
      }
    }
  }
}

void
Waveform::draw(void)
{
  bool overlaySel = false;

  if (!this->size().isValid())
    return;

  if (this->geometry != this->size()) {
    this->geometry = this->size();
    if (!this->haveGeometry) {
      this->haveGeometry = true;
      this->zoomVerticalReset();
      this->zoomHorizontalReset();
    }

    this->selectionPixmap = QPixmap(
          this->geometry.width(),
          this->geometry.height());

    this->axesPixmap = QPixmap(
          this->geometry.width(),
          this->geometry.height());

    this->contentPixmap = QPixmap(
          this->geometry.width(),
          this->geometry.height());

    this->waveform = QImage(this->geometry, QImage::Format_ARGB32);

    this->recalculateDisplayData();
    this->selectionDrawn = false;
    this->axesDrawn      = false;
    this->waveDrawn      = false;
  }

  //
  // Dirty selection implies to overlay axes and waveform
  // Dirty axes implies to redraw waveform
  //
  if (this->somethingDirty()) {
    if (!this->selectionDrawn) {
      this->drawSelection();
      overlaySel = true;
      this->selectionDrawn = true;
    }

    // Start from a fresh selection
    this->contentPixmap = this->selectionPixmap.copy(
          0,
          0,
          this->geometry.width(),
          this->geometry.height());

    QPainter p(&this->contentPixmap);

    if (overlaySel || !this->axesDrawn) {
      if (!this->axesDrawn) {
        this->drawAxes();
        this->axesDrawn = true;
        this->waveDrawn = false;
      }

      p.drawPixmap(0, 0, this->axesPixmap);
      overlaySel = true;
    }

    if (overlaySel || !this->waveDrawn) {
      if (!this->waveDrawn) {
        this->drawWave();
        this->waveDrawn = true;
      }

      if (this->hSelection && this->periodicSelection)
        p.setOpacity(.5);

      p.drawImage(0, 0, this->waveform);
      overlaySel = true;
    }

    if (overlaySel && this->hSelection)
      this->overlaySelectionMarkes(p);

    p.end();
  }
}

void
Waveform::paint(void)
{
  QPainter painter(this);
  painter.drawPixmap(0, 0, this->contentPixmap);

  if (this->haveCursor) {
    painter.setPen(this->axes);
    painter.drawLine(
          this->currMouseX,
          0,
          this->currMouseX,
          this->geometry.height() - 1);
  }

  painter.end();
}

void
Waveform::setData(const std::vector<SUCOMPLEX> *data)
{
  if (data != nullptr)
    this->data = WaveBuffer(data);
  else
    this->data = WaveBuffer();

  this->haveMagPhaseInfo = false;
  this->waveDrawn = false;
  this->resetSelection();
  this->zoomVerticalReset();
  this->zoomHorizontalReset();
  this->invalidate();
}

void
Waveform::setRealComponent(bool real)
{
  this->realComponent = real;
  this->waveDrawn = false;
  this->invalidate();
}

void
Waveform::setShowEnvelope(bool show)
{
  this->showEnvelope = show;
  this->waveDrawn = false;
  this->axesDrawn = false;
  this->invalidate();
}

void
Waveform::setShowPhase(bool show)
{
  this->showPhase = show;
  if (this->showEnvelope) {
    this->waveDrawn = false;
    this->axesDrawn = false;
    this->invalidate();
  }
}

void
Waveform::setShowPhaseDiff(bool show)
{
  this->showPhaseDiff = show;

  if (this->showEnvelope) {
    this->waveDrawn = false;
    this->axesDrawn = false;
    this->invalidate();
  }
}

void
Waveform::setPhaseDiffOrigin(unsigned origin)
{
  if (this->showEnvelope && this->showPhase && this->showPhaseDiff) {
    this->phaseDiffOrigin = origin & 0xff;
    this->waveDrawn = false;
    this->axesDrawn = false;
    this->invalidate();
  }
}


void
Waveform::setShowWaveform(bool show)
{
  this->showWaveform = show;
  this->waveDrawn = false;
  this->axesDrawn = false;
  this->invalidate();
}

void
Waveform::refreshData(void)
{
  qint64 currSpan = this->end - this->start;
  qint64 nextSpan = static_cast<qint64>(this->data.length());

  if (this->autoScroll && nextSpan > currSpan) {
    this->end = nextSpan - 1;
    this->start = nextSpan - currSpan - 1;
  } else {
    this->waveDrawn = false;
  }

  this->recalculateDisplayData();

  if (this->autoFitToEnvelope)
    this->fitToEnvelope();
}

Waveform::Waveform(QWidget *parent) :
  ThrottleableWidget(parent)
{
  unsigned int i;

  this->sampleRate = 1024000;
  this->deltaT = 1 / this->sampleRate;

  for (i = 0; i < 8192; ++i)
    this->data.feed(SU_ASFLOAT(.75) * SU_C_EXP(I * SU_ASFLOAT((M_PI * i) / 64)));

  for (int i = 0; i < 256; i++) {
    // level 0: black background
    if (i < 20)
      colorTable[i].setRgb(0, 0, 0);
    // level 1: black -> blue
    else if ((i >= 20) && (i < 70))
      colorTable[i].setRgb(0, 0, 140*(i-20)/50);
    // level 2: blue -> light-blue / greenish
    else if ((i >= 70) && (i < 100))
      colorTable[i].setRgb(60*(i-70)/30, 125*(i-70)/30, 115*(i-70)/30 + 140);
    // level 3: light blue -> yellow
    else if ((i >= 100) && (i < 150))
      colorTable[i].setRgb(195*(i-100)/50 + 60, 130*(i-100)/50 + 125, 255-(255*(i-100)/50));
    // level 4: yellow -> red
    else if ((i >= 150) && (i < 250))
      colorTable[i].setRgb(255, 255-255*(i-150)/100, 0);
    // level 5: red -> white
    else if (i >= 250)
      colorTable[i].setRgb(255, 255*(i-250)/5, 255*(i-250)/5);
  }

  this->background   = WAVEFORM_DEFAULT_BACKGROUND_COLOR;
  this->foreground   = WAVEFORM_DEFAULT_FOREGROUND_COLOR;
  this->axes         = WAVEFORM_DEFAULT_AXES_COLOR;
  this->text         = WAVEFORM_DEFAULT_TEXT_COLOR;
  this->selection    = WAVEFORM_DEFAULT_SELECTION_COLOR;
  this->subSelection = WAVEFORM_DEFAULT_SUBSEL_COLOR;
  this->envelope     = WAVEFORM_DEFAULT_ENVELOPE_COLOR;
  this->setMouseTracking(true);
  this->invalidate();
}
