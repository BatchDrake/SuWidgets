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
#include <QPainterPath>
#include <QColormap>
#include <SuWidgetsHelpers.h>

////////////////////////// WaveBuffer methods //////////////////////////////////
WaveBuffer::WaveBuffer(WaveView *view)
{
  this->view = view;
  this->buffer = &this->ownBuffer;

  if (view != nullptr)
    this->view->build(this->buffer->data(), this->buffer->size());
}

WaveBuffer::WaveBuffer(WaveView *view, const std::vector<SUCOMPLEX> *vec)
{
  this->view = view;
  this->loan = true;
  this->buffer = vec;

  if (view != nullptr)
    this->view->build(this->buffer->data(), this->buffer->size());
}

bool
WaveBuffer::feed(SUCOMPLEX val)
{
  SUSCOUNT since;

  if (this->loan)
    return false;

  since = this->ownBuffer.size();

  this->ownBuffer.push_back(val);

  if (view != nullptr)
  this->view->build(this->ownBuffer.data(), this->ownBuffer.size(), since);

  return true;
}

void
WaveBuffer::rebuildViews(void)
{
  if (this->view != nullptr) {
    SUSCOUNT since = this->view->getLength();

    if (since < this->buffer->size())
      this->view->build(this->buffer->data(), this->buffer->size(), since);
  }
}

bool
WaveBuffer::feed(std::vector<SUCOMPLEX> const &vec)
{
  SUSCOUNT since;

  if (this->loan)
    return false;

  since = this->ownBuffer.size();

  this->ownBuffer.insert(this->ownBuffer.end(), vec.begin(), vec.end());

  if (view != nullptr)
    this->view->build(this->ownBuffer.data(), this->ownBuffer.size(), since);

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

const std::vector<SUCOMPLEX> *
WaveBuffer::loanedBuffer(void) const
{
  if (!this->loan)
    return nullptr;

  return this->buffer;
}

////////////////////////// Geometry methods ////////////////////////////////////
void
Waveform::recalculateDisplayData(void)
{
  qreal range;
  qreal divLen;

  // In every direction must be a minimum of 5 divisions, and a maximum of 10
  //
  // 7.14154 - 7.143. Range is 0.00146.
  // First significant digit: at 1e-3
  //  7.141 ... 2 ...  3. Only 3 divs if rounding to the 1e-3.
  //  7.141 ...15 ... 20 ... 25 ... 30. 5 divs if rounding to de 5e-4!

  range = this->view.getViewInterval();
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

  this->hDivSamples = divLen * this->view.getSampleRate();

  // Same procedure for vertical axis
  range = this->view.getViewRange();
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
}

void
Waveform::zoomHorizontalReset(void)
{
  if (this->haveGeometry)
    this->zoomHorizontal(0, static_cast<qint64>(this->data.length()) - 1);
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
  newRange  = std::ceil(amount * this->view.getViewSampleInterval());

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
  qint64 currStart = this->getSampleStart();
  qint64 currEnd   = this->getSampleEnd();

  if (start != currStart || end != currEnd) {
    this->view.setHorizontalZoom(start, end);
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
  this->savedStart = this->getSampleStart();
  this->savedEnd   = this->getSampleEnd();
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
        this->savedStart - static_cast<qint64>(delta * this->getSamplesPerPixel()),
        this->savedEnd   - static_cast<qint64>(delta * this->getSamplesPerPixel()));
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
        (this->getMin() - val) * amount + val,
        (this->getMax() - val) * amount + val);
}

void
Waveform::zoomVertical(qreal min, qreal max)
{
  this->view.setVerticalZoom(min, max);
  this->axesDrawn = false;

  this->recalculateDisplayData();
  emit verticalRangeChanged(min, max);
}

void
Waveform::saveVertical(void)
{
  this->savedMin = this->getMin();
  this->savedMax = this->getMax();
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
        this->savedMin + delta * this->getUnitsPerPx(),
        this->savedMax + delta * this->getUnitsPerPx());
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
  qreal envelope = this->view.getEnvelope();

  if (envelope > 0)
    this->zoomVertical(-envelope, envelope);
  else
    this->zoomVerticalReset();
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
  // In some barely-reproducible cases, the first scroll produces a
  // delta value that is so big that jumps straight to the maximum
  // zoom level. The causes for this are yet to be determined.

  if (event->delta() >= -WAVEFORM_DELTA_LIMIT
      && event->delta() <= WAVEFORM_DELTA_LIMIT) {
    qreal amount = std::pow(
          static_cast<qreal>(1.1),
          static_cast<qreal>(-event->delta() / 120.));
    if (event->x() < this->valueTextWidth)
      this->zoomVertical(static_cast<qint64>(event->y()), amount);
    else
      this->zoomHorizontal(static_cast<qint64>(event->x()), amount);

    this->invalidate();
  }
}

void
Waveform::leaveEvent(QEvent *)
{
  this->haveCursor = false;
  this->invalidate();
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
    qreal selLen = this->hSelEnd - this->hSelStart;
    QRect rect(
          static_cast<int>(this->samp2px(this->hSelStart)),
          0,
          static_cast<int>(selLen / this->getSamplesPerPixel()),
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
  waveform.fill(Qt::transparent);
  QPainter p(&this->waveform);

  this->view.drawWave(p);

  if (this->markerList.size() > 0) {
    QFont font;
    QFontMetrics metrics(font);
    QPen pen(this->text);
    QRect rect;

    p.setPen(pen);

    for (auto m = this->markerList.begin();
         m != this->markerList.end();
         ++m)
    {
      int tw;
      qint64 px = SCAST(qint64, this->samp2px(m->x));

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
      tw = metrics.horizontalAdvance(m->string);
#else
      tw = metrics.width(m->string);
#endif // QT_VERSION_CHECK

      if (px >= 0 && px < this->geometry.width() - tw / 2) {
        qreal y = m->x < this->getDataLength()
            ? this->cast(this->getData()[m->x])
            : 0;
        int ypx = this->value2px(y) +
            (m->below ? 2 : - metrics.height() - 2);

        ypx = qBound(0, ypx, this->geometry.height() - metrics.height());

        rect.setRect(
              px - tw / 2,
              ypx,
              tw,
              metrics.height());
        p.setOpacity(1);
        p.drawText(rect, Qt::AlignHCenter | Qt::AlignBottom,m->string);
      }
    }
  }

  p.end();
}

void
Waveform::drawVerticalAxes(void)
{
  QFont font;
  QPainter p(&this->axesPixmap);
  QFontMetrics metrics(font);
  QRect rect;
  QPen pen(this->axes);
  qreal deltaT = this->view.getDeltaT();
  int axis;
  int px;

  pen.setStyle(Qt::DotLine);
  p.setPen(pen);
  p.setFont(font);

  this->frequencyTextHeight = metrics.height();

  if (this->hDivSamples > 0) {
    qreal rem = this->oX -
        this->hDivSamples * std::floor(this->oX / this->hDivSamples);

    // Draw axes
    axis = static_cast<int>(std::floor(this->getSampleStart() / this->hDivSamples));

    while (axis * this->hDivSamples <= this->getSampleEnd() + rem) {
      px = static_cast<int>(this->samp2px(axis * this->hDivSamples - rem));

      if (px > 0)
        p.drawLine(px, 0, px, this->geometry.height() - 1);
      ++axis;
    }

    // Draw labels
    p.setPen(this->text);
    axis = static_cast<int>(std::floor(this->getSampleStart() / this->hDivSamples));
    while (axis * this->hDivSamples <= this->getSampleEnd() + rem) {
      px = static_cast<int>(this->samp2px(axis * this->hDivSamples - rem));

      if (px > 0) {
        QString label;
        int tw;

        label = SuWidgetsHelpers::formatQuantityFromDelta(
              (this->oX + axis * this->hDivSamples - rem) * deltaT,
              this->hDivSamples * deltaT,
              this->horizontalUnits);

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        tw = metrics.horizontalAdvance(label);
#else
        tw = metrics.width(label);
#endif // QT_VERSION_CHECK

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
    axis = static_cast<int>(std::floor(this->getMin() / this->vDivUnits));

    while (axis * this->vDivUnits <= this->getMax()) {
      pen.setStyle(axis == 0 ? Qt::SolidLine : Qt::DotLine);
      p.setPen(pen);
      px = static_cast<int>(this->value2px(axis * this->vDivUnits));

      if (px > 0)
        p.drawLine(0, px, this->geometry.width() - 1, px);
      ++axis;
    }

    p.setPen(this->text);
    axis = static_cast<int>(std::floor(this->getMin() / this->vDivUnits));
    while (axis * this->vDivUnits <= this->getMax()) {
      px = static_cast<int>(this->value2px(axis * this->vDivUnits));

      if (px > 0) {
        QString label;
        int tw;

        label = SuWidgetsHelpers::formatQuantityFromDelta(
              axis * this->vDivUnits,
              this->vDivUnits,
              this->verticalUnits);

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        tw = metrics.horizontalAdvance(label);
#else
        tw = metrics.width(label);
#endif // QT_VERSION_CHECK

        rect.setRect(
              0,
              px - metrics.height() / 2,
              tw,
              metrics.height());

        if (tw > this->valueTextWidth) {
          this->valueTextWidth = tw;
          this->view.setLeftMargin(tw);
        }

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
    qint64 firstDiv = static_cast<int>((this->getSampleStart() - this->hSelStart) / selLen);
    qint64 lastDiv  = static_cast<int>((this->getSampleEnd() - this->hSelStart) / selLen);

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

  // This is not valid either
  if (this->size().width() * this->size().height() < 1)
    return;

  if (this->geometry != this->size()) {
    this->view.setGeometry(this->size().width(), this->size().height());

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
Waveform::reuseDisplayData(Waveform *other)
{
  this->view.borrowTree(other->view);
}

void
Waveform::setData(
    const std::vector<SUCOMPLEX> *data,
    bool keepView,
    bool flush)
{
  bool   appending  = data != nullptr && data == this->data.loanedBuffer();
  qint64 prevLength = SCAST(qint64, this->view.getLength());
  qint64 newLength  = data == nullptr ? 0 : static_cast<qint64>(data->size());
  qint64 extra      = newLength - prevLength;

  if (appending) {
    // The current wavebuffer holds a reference to the same data vector,
    // we only inform the view to recalculate the new samples
    if (flush) {
      this->view.build(data->data(), data->size(), 0);
    } else if (extra > 0) {
      this->data.rebuildViews();
    }
  } else {
    this->view.flush();

    if (data != nullptr)
      this->data = WaveBuffer(&this->view, data);
    else
      this->data = WaveBuffer(&this->view);
  }

  this->waveDrawn = false;

  if (!keepView) {
    this->resetSelection();
    this->zoomVerticalReset();
    this->zoomHorizontalReset();
  } else {
    this->axesDrawn = false;
    this->selectionDrawn = false;
  }

  this->invalidate();
}

void
Waveform::setRealComponent(bool real)
{
  this->view.setRealComponent(real);
  this->fitToEnvelope();
  this->invalidate();
}

void
Waveform::setShowEnvelope(bool show)
{
  this->view.setShowEnvelope(show);
  this->waveDrawn = false;
  this->axesDrawn = false;
  this->invalidate();
}

void
Waveform::setShowPhase(bool show)
{
  this->view.setShowPhase(show);
  if (this->view.isEnvelopeVisible()) {
    this->waveDrawn = false;
    this->axesDrawn = false;
    this->invalidate();
  }
}

void
Waveform::setShowPhaseDiff(bool show)
{
  this->view.setShowPhaseDiff(show);

  if (this->view.isEnvelopeVisible()) {
    this->waveDrawn = false;
    this->axesDrawn = false;
    this->invalidate();
  }
}

void
Waveform::setPhaseDiffOrigin(unsigned origin)
{
  this->view.setPhaseDiffOrigin(origin);

  if (this->view.isEnvelopeVisible()
      && this->view.isPhaseEnabled()
      && this->view.isPhaseDiffEnabled()) {
    this->waveDrawn = false;
    this->axesDrawn = false;
    this->invalidate();
  }
}

void
Waveform::setPhaseDiffContrast(qreal contrast)
{
  this->view.setPhaseDiffContrast(contrast);

  if (this->view.isEnvelopeVisible()
      && this->view.isPhaseEnabled()
      && this->view.isPhaseDiffEnabled()) {
    this->waveDrawn = false;
    this->axesDrawn = false;
    this->invalidate();
  }
}

void
Waveform::setShowWaveform(bool show)
{
  this->view.setShowWaveform(show);

  this->waveDrawn = false;
  this->axesDrawn = false;
  this->invalidate();
}

void
Waveform::refreshData(void)
{
  qint64 currSpan = this->view.getViewSampleInterval();
  qint64 lastSample = static_cast<qint64>(this->data.length()) - 1;

  this->view.flush();
  this->view.build(this->getData(), this->getDataLength(), 0);

  if (this->autoScroll && this->getSampleEnd() <= lastSample)
    this->view.setHorizontalZoom(lastSample - currSpan, lastSample);

  this->waveDrawn = false;

  this->recalculateDisplayData();

  if (this->autoFitToEnvelope)
    this->fitToEnvelope();
  else
    this->axesDrawn = false;
}

Waveform::Waveform(QWidget *parent) :
  ThrottleableWidget(parent),
  data(&this->view)
{
  std::vector<QColor> colorTable;

  this->view.setTimeUnits(0, 1024000);

  colorTable.resize(256);

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

  this->view.setPalette(colorTable.data());
  this->view.setForeground(this->foreground);

  this->setMouseTracking(true);
  this->invalidate();
}
