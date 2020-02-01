//
//    filename: description
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
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
  this->start = 0;

  if (this->haveGeometry) {
    if (this->data.length() <= static_cast<unsigned>(this->geometry.width()))
      this->end = this->geometry.width() - 1;
    else
      this->end = static_cast<int>(this->data.length()) - 1;
  }

  this->recalculateDisplayData();
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

  fixedSamp = std::ceil(this->px2samp(x));
  newRange = std::ceil(amount * (this->end - this->start));

  this->start = static_cast<qint64>(fixedSamp - relPoint * newRange);
  this->end   = static_cast<qint64>(fixedSamp + (1.0 - relPoint) * newRange);
  this->axesDrawn = false;

  this->recalculateDisplayData();
}

void
Waveform::zoomHorizontal(qreal tStart, qreal tEnd)
{
  this->start = static_cast<qint64>(this->t2samp(tStart));
  this->end   = static_cast<qint64>(this->t2samp(tEnd));
  this->axesDrawn = false;

  this->recalculateDisplayData();
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
  this->start = this->savedStart - static_cast<qint64>(delta * this->sampPerPx);
  this->end   = this->savedEnd   - static_cast<qint64>(delta * this->sampPerPx);
  this->axesDrawn = false;

  this->recalculateDisplayData();
}

void
Waveform::selectHorizontal(qint64 orig, qint64 to)
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
}

void
Waveform::zoomVerticalReset(void)
{
  this->min = -1;
  this->max =  1;
  this->axesDrawn = false;

  this->recalculateDisplayData();
}

void
Waveform::zoomVertical(qint64 y, qreal amount)
{
  qreal val = this->px2value(y);

  this->min = (this->min - val) * amount + val;
  this->max = (this->max - val) * amount + val;
  this->axesDrawn = false;

  this->recalculateDisplayData();
}

void
Waveform::zoomVertical(qreal start, qreal end)
{
  this->min = start;
  this->max = end;
  this->axesDrawn = false;

  this->recalculateDisplayData();
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
  this->min = this->savedMin + delta * this->unitsPerPx;
  this->max = this->savedMax + delta * this->unitsPerPx;
  this->axesDrawn = false;

  this->recalculateDisplayData();
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

  this->invalidateHard();
}

void
Waveform::mousePressEvent(QMouseEvent *event)
{
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

  if (event->x() < this->valueTextWidth)
    this->zoomVertical(
        static_cast<qint64>(event->y()),
        event->delta() < 0 ? 1.1 : 0.9);
  else
    this->zoomHorizontal(
        static_cast<qint64>(event->x()),
        event->delta() < 0 ? 1.1 : 0.9);
  this->invalidateHard();
}

void
Waveform::leaveEvent(QEvent *)
{
  this->haveCursor = false;
}

//////////////////////////////// Drawing methods ///////////////////////////////////
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
    QRect rect(
          static_cast<int>(this->samp2px(this->hSelStart)),
          0,
          static_cast<int>(
            this->samp2px(this->hSelEnd) - this->samp2px(this->hSelStart)),
          this->geometry.height());

    if (rect.x() < this->valueTextWidth)
      rect.setX(this->valueTextWidth);

    if (rect.right() >= this->geometry.width())
      rect.setRight(this->geometry.width() - 1);


    p.fillRect(rect, QColor(0x08, 0x08, 0x08));
  }

  p.end();
}

void
Waveform::drawWave(void)
{
  const SUCOMPLEX *data = this->data.data();
  int length = static_cast<int>(this->data.length());
  QPainter p(&this->contentPixmap);
  QPen pen(this->foreground);
  // Two cases: if sampPerPx > 1: create small history of samples. Otherwise,
  // just interpolate

  pen.setStyle(Qt::SolidLine);
  p.setPen(pen);

  if (this->sampPerPx > 1) {
    std::vector<int> history(static_cast<size_t>(this->geometry.height()));
    int iters = static_cast<int>(std::floor(this->sampPerPx));
    int samp = 0;
    int y = 0;
    int prev_y;
    bool havePrev = false;

    for (int i = this->valueTextWidth; i < this->geometry.width(); ++i) {
      std::fill(history.begin(), history.end(), 0);
      samp = static_cast<int>(std::floor(this->px2samp(i)));

      if (samp >= 0 && samp < length - iters) {
        for (int j = 0; j < iters; ++j) {
          y = static_cast<int>(this->value2px(this->cast(data[samp + j])));
          if (havePrev)
            for (int k = std::min(y, prev_y); k <= std::max(y, prev_y); ++k)
              if (k >= 0 && k < this->geometry.height())
              ++history[static_cast<unsigned>(k)];
          havePrev = true;
          prev_y = y;
        }

        for (int j = 0; j < this->geometry.height(); ++j) {
          if (history[static_cast<unsigned>(j)] > 0) {
            p.setOpacity(history[static_cast<unsigned>(j)] / static_cast<qreal>(iters));
            p.drawPoint(i, j);
          }
        }
      }
    }

  } else {
    qreal firstSamp, lastSamp;
    int firstIntegerSamp, lastIntegerSamp;

    firstSamp = this->px2samp(this->valueTextWidth);
    lastSamp  = this->px2samp(this->geometry.width() - 1);

    firstIntegerSamp = static_cast<int>(std::ceil(firstSamp));
    lastIntegerSamp  = static_cast<int>(std::floor(lastSamp));

    for (int i = firstIntegerSamp; i < lastIntegerSamp; ++i)
      if (i >= 0 && i < length - 1) {
        p.drawLine(
              static_cast<int>(this->samp2px(i)),
              static_cast<int>(this->value2px(this->cast(data[i]))),
              static_cast<int>(this->samp2px(i + 1)),
              static_cast<int>(this->value2px(this->cast(data[i + 1]))));
    }
  }

  p.end();
}

QString
Waveform::formatLabel(qreal value, int digits, QString units)
{
  qreal multiplier = 1;
  QString subUnits[] = {
    units,
    "m" + units,
    "μ" + units,
    "n" + units,
    "p" + units,
    "f" + units};
  QString num;
  int i = 0;

  while (i++ < 6 && digits < -1) {
    multiplier *= 1e3;
    digits += 3;
  }

  if (digits >= 0) {
    if (digits > 2 && units == "s") { // This is too long. Format to minutes and seconds
      char time[64];
      int seconds = static_cast<int>(value);
      QString sign;
      if (seconds < 0) {
        seconds = 0;
        sign = "-";
      }
      int minutes = seconds / 60;
      int hours   = seconds / 3600;

      seconds %= 60;
      minutes %= 60;

      if (hours > 0)
        snprintf(time, sizeof(time), "%02d:%02d:%0d", hours, minutes, seconds);
      else
        snprintf(time, sizeof(time), "%02d:%0d", minutes, seconds);
      return sign + QString(time);
    } else {
      digits = 0;
    }
  }

  num.setNum(value * multiplier, 'f', -digits);

  num += " " + subUnits[i - 1];

  if (units != "s" && value > 0)
    num = "+" + num;

  return num;
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

        label = formatLabel(
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

        label = formatLabel(
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
Waveform::draw(void)
{
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

    this->recalculateDisplayData();
    this->selectionDrawn = false;
  }

  if (!this->selectionDrawn) {
    this->drawSelection();
    this->axesDrawn = false;
  }

  if (!this->axesDrawn) {
    this->drawAxes();
    this->selectionDrawn = false;
    this->axesDrawn = true;
    this->waveDrawn = false;
    QPainter p(&this->selectionPixmap);
    p.drawPixmap(QPoint(0, 0), this->axesPixmap);
    p.end();
  }

  if (!this->waveDrawn) {
    this->contentPixmap = this->selectionPixmap.copy(
          0,
          0,
          this->geometry.width(),
          this->geometry.height());


    this->drawWave();
    this->waveDrawn = true;
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
}

Waveform::Waveform(QWidget *parent) :
  ThrottleableWidget(parent)
{
  unsigned int i;

  this->sampleRate = 1024000;
  this->deltaT = 1 / this->sampleRate;

  for (i = 0; i < 8192; ++i)
    this->data.feed(.75 * sin((M_PI * i) / 64));

  this->background = WAVEFORM_DEFAULT_BACKGROUND_COLOR;
  this->foreground = WAVEFORM_DEFAULT_FOREGROUND_COLOR;
  this->axes       = WAVEFORM_DEFAULT_AXES_COLOR;
  this->text       = WAVEFORM_DEFAULT_TEXT_COLOR;

  this->setMouseTracking(true);

  this->invalidate();
}
