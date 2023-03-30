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
#include <QApplication>
#include <SuWidgetsHelpers.h>
#include <assert.h>

////////////////////////// WaveBuffer methods //////////////////////////////////
void
WaveBuffer::operator=(const WaveBuffer &prev)
{
  m_view      = prev.m_view;
  m_ownBuffer = prev.m_ownBuffer;
  m_loan      = prev.m_loan;
  m_ro        = prev.m_ro;

  m_ro_data   = prev.m_ro_data;
  m_ro_size   = prev.m_ro_size;

  if (!isLoan())
    m_buffer = &m_ownBuffer;
  else
    m_buffer = prev.m_buffer;
}

// Constructor by allocation of new buffer
WaveBuffer::WaveBuffer(WaveView *view)
{
  m_view   = view;
  m_buffer = &m_ownBuffer;
  m_loan   = false;
  m_ro     = false;

  assert(isLoan() || m_buffer == &m_ownBuffer);

  updateBuffer();
}

// Constructor by loan (read / write)
WaveBuffer::WaveBuffer(WaveView *view, const std::vector<SUCOMPLEX> *vec)
{
  m_view   = view;
  m_loan   = true;
  m_ro     = false;
  m_buffer = vec;

  updateBuffer();
}

// Constructor by loan (read only)
WaveBuffer::WaveBuffer(WaveView *view, const SUCOMPLEX *data, size_t size)
{
  m_view    = view;
  m_buffer  = nullptr;
  m_loan    = true;
  m_ro      = true;

  m_ro_data = data;
  m_ro_size = size;

  updateBuffer();
}

bool
WaveBuffer::feed(SUCOMPLEX val)
{
  if (m_loan)
    return false;

  m_ownBuffer.push_back(val);

  if (m_view != nullptr)
    m_view->refreshBuffer(&m_ownBuffer);

  return true;
}

void
WaveBuffer::rebuildViews()
{
  refreshBufferCache();

  if (m_view != nullptr)
    m_view->refreshBuffer(m_ro_data, m_ro_size);
}

bool
WaveBuffer::feed(std::vector<SUCOMPLEX> const &vec)
{
  if (m_loan)
    return false;

  m_ownBuffer.insert(m_ownBuffer.end(), vec.begin(), vec.end());
  refreshBufferCache();

  if (m_view != nullptr)
    m_view->refreshBuffer(&m_ownBuffer);

  return true;
}

size_t
WaveBuffer::length() const
{
  assert(isLoan() || m_buffer == &m_ownBuffer);

  return m_ro ? m_ro_size : m_buffer->size();
}

const SUCOMPLEX *
WaveBuffer::data() const
{
  assert(this->isLoan() || m_buffer == &m_ownBuffer);

  return m_ro ? m_ro_data : m_buffer->data();
}

const std::vector<SUCOMPLEX> *
WaveBuffer::loanedBuffer() const
{
  if (!m_loan)
    return nullptr;

  if (m_ro)
    return nullptr;

  return m_buffer;
}

////////////////////////// Geometry methods ////////////////////////////////////
void
Waveform::recalculateDisplayData()
{
  qreal range;
  qreal divLen;

  // In every direction must be a minimum of 5 divisions, and a maximum of 10
  //
  // 7.14154 - 7.143. Range is 0.00146.
  // First significant digit: at 1e-3
  //  7.141 ... 2 ...  3. Only 3 divs if rounding to the 1e-3.
  //  7.141 ...15 ... 20 ... 25 ... 30. 5 divs if rounding to de 5e-4!

  range = m_view.getViewInterval();
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

  this->hDivSamples = divLen * m_view.getSampleRate();

  // Same procedure for vertical axis
  range = m_view.getViewRange();
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
Waveform::zoomHorizontalReset()
{
  if (this->haveGeometry) {
    qint64 length = SCAST(qint64, m_data.length());

    if (length > 0)
      this->zoomHorizontal(SCAST(qint64, 0), length - 1);
    else if (this->getSampleRate() > 0)
      this->zoomHorizontal(
          SCAST(qint64, 0),
          SCAST(qint64, this->getSampleRate()));
    else
      this->zoomHorizontal(SCAST(qint64, 0), SCAST(qint64, 0));
  }
}

void
Waveform::zoomHorizontal(qint64 x, qreal amount)
{
  qreal newRange;
  qreal fixedSamp;
  qreal relPoint = static_cast<qreal>(x - this->valueTextWidth) / (this->geometry.width());

  //
  // This means that position at x remains the same, while the others shrink
  // or stretch accordingly
  //

  fixedSamp = std::round(this->px2samp(x));
  newRange  = std::ceil(amount * m_view.getViewSampleInterval());

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
    m_view.setHorizontalZoom(start, end);
    if (this->hSelection)
      this->selUpdated = false;
    this->axesDrawn = false;
    this->recalculateDisplayData();
    emit horizontalRangeChanged(start, end);
  }
}

void
Waveform::saveHorizontal()
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

  this->selUpdated = false;

  emit horizontalSelectionChanged(this->hSelStart, this->hSelEnd);
}

bool
Waveform::getHorizontalSelectionPresent() const
{
  return this->getDataLength() > 0 && this->hSelection;
}

qreal
Waveform::getHorizontalSelectionStart() const
{
  if (!this->getHorizontalSelectionPresent())
    return .0;
  else
    return qBound(.0, this->hSelStart, SCAST(qreal, this->getDataLength() - 1));
}

qreal
Waveform::getHorizontalSelectionEnd() const
{
  if (!this->getHorizontalSelectionPresent())
    return .0;
  else
    return qBound(.0, this->hSelEnd, SCAST(qreal, this->getDataLength() - 1));
}

void
Waveform::setAutoScroll(bool value)
{
  this->autoScroll = value;
  this->refreshData();
}

void
Waveform::zoomVerticalReset()
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
  m_view.setVerticalZoom(min, max);
  this->axesDrawn = false;

  this->recalculateDisplayData();
  emit verticalRangeChanged(min, max);
}

void
Waveform::saveVertical()
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

  this->selUpdated = false;

  emit verticalSelectionChanged(this->vSelStart, this->vSelEnd);
}

bool
Waveform::getVerticalSelectionPresent() const
{
  return this->vSelection;
}

qreal
Waveform::getVerticalSelectionStart() const
{
  return this->vSelStart;
}

qreal
Waveform::getVerticalSelectionEnd() const
{
  return this->vSelEnd;
}

void
Waveform::fitToEnvelope()
{
  qreal envelope = m_view.getEnvelope();

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
Waveform::resetSelection()
{
  this->hSelection = this->vSelection = false;
  this->selUpdated = false;
}

void
Waveform::setPeriodicSelection(bool val)
{
  this->periodicSelection = val;
  this->selUpdated = false;
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
          static_cast<qint64>(this->px2samp(this->samp2px(this->clickSample))),
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

    this->clickSample = this->px2samp(this->clickX);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (event->button() == Qt::MiddleButton
#else
    if (event->button() == Qt::MidButton
#endif // QT_VERSION
        || this->clickY >= this->geometry.height() - this->frequencyTextHeight)
      this->frequencyDragging = true;
    else if (this->clickX < this->valueTextWidth)
      this->valueDragging = !this->autoFitToEnvelope && true;
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
Waveform::mouseDoubleClickEvent(QMouseEvent *event)
{
  int x = event->pos().x();
  int y = event->pos().y();
  qreal t = px2samp(x);
  qreal v = px2value(y);

  emit pointClicked(t, v, event->modifiers());
}

bool
Waveform::event(QEvent *event)
{
  if (event->type() == QEvent::ToolTip) {
    QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);

    int x = helpEvent->globalPos().x();
    int y = helpEvent->globalPos().y();

    qreal t = px2t(helpEvent->pos().x());
    qreal v = px2value(helpEvent->pos().y());

    emit toolTipAt(x, y, t, v);

    return true;
  }

  return QWidget::event(event);
}

void
Waveform::wheelEvent(QWheelEvent *event)
{
  int delta = event->angleDelta().y();
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    int x = SCAST(int, event->position().x());
    int y = SCAST(int, event->position().y());
#else
    int x = event->x();
    int y = event->y();
#endif // QT_VERSION

  // In some barely-reproducible cases, the first scroll produces a
  // delta value that is so big that jumps straight to the maximum
  // zoom level. The causes for this are yet to be determined.

  if (delta >= -WAVEFORM_DELTA_LIMIT
      && delta <= WAVEFORM_DELTA_LIMIT) {
    qreal amount = std::pow(
          static_cast<qreal>(1.1),
          static_cast<qreal>(-delta / 120.));
    if (x < this->valueTextWidth) {
      if (!this->autoFitToEnvelope)
        this->zoomVertical(static_cast<qint64>(y), amount);
    } else {
      this->zoomHorizontal(static_cast<qint64>(x), amount);
    }

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
Waveform::overlaySelection(QPainter &p)
{
  if (this->hSelection) {
    // qreal selLen = this->hSelEnd - this->hSelStart;
    int xStart = SCAST(int, this->samp2px(this->hSelStart));
    int xEnd   = SCAST(int, this->samp2px(this->hSelEnd));

    QRect rect1(
          0,
          0,
          xStart,
          this->geometry.height());

    QRect rect2(
          xEnd,
          0,
          this->geometry.width() - xEnd,
          this->geometry.height());

    if (rect1.x() < this->valueTextWidth)
      rect1.setX(this->valueTextWidth);

    if (rect1.right() >= this->geometry.width())
      rect1.setRight(this->geometry.width() - 1);

    if (rect2.x() < this->valueTextWidth)
      rect2.setX(this->valueTextWidth);

    if (rect2.right() >= this->geometry.width())
      rect2.setRight(this->geometry.width() - 1);


    p.save();
    p.setOpacity(.5);
    p.fillRect(rect1, this->selection);
    p.fillRect(rect2, this->selection);
    p.restore();
  }
}

void
Waveform::overlayMarkers(QPainter &p)
{
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
}

void
Waveform::overlayVCursors(QPainter &p)
{
  if (this->vCursorList.size() > 0) {
    int width = p.device()->width();
    QFont font;
    QFontMetrics metrics(font);
    QPen pen;
    int x = this->valueTextWidth;

    p.save();

    pen.setStyle(Qt::DashLine);
    pen.setWidth(1);

    p.setOpacity(1);

    for (auto c = this->vCursorList.begin();
         c != this->vCursorList.end();
         ++c) {
      QPainterPath path;
      int y = SCAST(int, this->value2px(this->cast(c->level)));

      path.moveTo(x + 10, y);
      path.lineTo(x, y - 5);
      path.lineTo(x, y + 5);
      path.lineTo(x + 10, y);

      p.setPen(Qt::NoPen);
      p.fillPath(path, QBrush(c->color));

      pen.setColor(c->color);
      p.setPen(pen);

      p.drawText(x + 10, y - metrics.height() / 2, c->string);
      p.drawLine(x + 10, y, width - 1, y);
    }

    p.restore();
  }
}

void
Waveform::overlayACursors(QPainter &p)
{
  if (this->aCursorList.size() > 0) {
    QFont font;
    QFontMetrics metrics(font);
    int x = this->valueTextWidth;
    int width = p.device()->width();

    p.save();

    p.setOpacity(1);

    for (auto a = this->aCursorList.begin();
         a != this->aCursorList.end();
         ++a) {
      QPainterPath path;
      QPen pen;
      int y1 = SCAST(int, this->value2px(+a->amplitude));
      int y2 = SCAST(int, this->value2px(-a->amplitude));

      pen.setWidth(1);
      pen.setColor(a->color);

      p.setPen(pen);

      p.drawText(x, y1 - metrics.height() / 2, a->string);
      p.fillRect(x, y1, width - x, y2 - y1 + 2, a->color);
    }

    p.restore();
  }
}

void
Waveform::drawWave()
{
  waveform.fill(Qt::transparent);
  QPainter p(&this->waveform);

  this->overlayACursors(p);
  m_view.drawWave(p);
  this->overlayMarkers(p);
  this->overlayVCursors(p);
  p.end();
}

void
Waveform::drawVerticalAxes()
{
  QFont font;
  QPainter p(&this->axesPixmap);
  QFontMetrics metrics(font);
  QRect rect;
  QPen pen(this->axes);
  qreal deltaT = m_view.getDeltaT();
  int axis;
  int px;
  bool unixTime = this->horizontalUnits == "unix";
  int previousLabel = -1;

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

        if (unixTime)
          label = SuWidgetsHelpers::formatQuantity(
                (this->oX + axis * this->hDivSamples - rem) * deltaT + m_view.samp2t(0),
                0,
                this->horizontalUnits);
        else
          label = SuWidgetsHelpers::formatQuantityFromDelta(
                (this->oX + axis * this->hDivSamples - rem) * deltaT,
                this->hDivSamples * deltaT,
                this->horizontalUnits);

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        tw = metrics.horizontalAdvance(label);
#else
        tw = metrics.width(label);
#endif // QT_VERSION_CHECK

        if (previousLabel == -1 || previousLabel < px - tw / 2) {
          rect.setRect(
                px - tw / 2,
                this->geometry.height() - this->frequencyTextHeight,
                tw,
                this->frequencyTextHeight);
          p.drawText(rect, Qt::AlignHCenter | Qt::AlignBottom, label);
          previousLabel = px + tw / 2;
        }
      }
      ++axis;
    }
  }
  p.end();
}

void
Waveform::drawHorizontalAxes()
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
          m_view.setLeftMargin(tw);
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
Waveform::drawAxes()
{
  this->axesPixmap.fill(Qt::transparent);
  this->drawHorizontalAxes();
  this->drawVerticalAxes();
}

void
Waveform::overlaySelectionMarkes(QPainter &p)
{
  int xStart = SCAST(int, this->samp2px(this->hSelStart));
  int xEnd   = SCAST(int, this->samp2px(this->hSelEnd));

  if (this->periodicSelection) {
    qreal selLen   = this->hSelEnd - this->hSelStart;
    qreal deltaDiv = selLen / static_cast<qreal>(this->divsPerSelection);
    bool manyLines = deltaDiv <= this->getSamplesPerPixel();

    if (manyLines) {
      QRect rect(
            xStart,
            0,
            xEnd - xStart,
            this->geometry.height() - this->frequencyTextHeight);

      if (rect.x() < this->valueTextWidth)
        rect.setX(this->valueTextWidth);

      if (rect.right() >= this->geometry.width())
        rect.setRight(this->geometry.width() - 1);

      p.setOpacity(.5);
      p.fillRect(rect, this->subSelection);
    } else {
      QPen pen;
      pen.setStyle(Qt::DashLine);
      pen.setColor(this->subSelection);

      p.setOpacity(1);

      for (int i = 0; i <= this->divsPerSelection; ++i) {
        qreal divSample = i * deltaDiv + this->hSelStart;
        int px = static_cast<int>(this->samp2px(divSample));

        p.setPen(pen);

        if (px > this->valueTextWidth && px < this->geometry.width())
          p.drawLine(
                px,
                0,
                px,
                this->geometry.height() - this->frequencyTextHeight);
      }
    }
  } else {
    QPen pen;
    QRect rect;
    QFont font;
    QFontMetrics metrics(font);
    int tw;
    qint64 px = xEnd;
    int ypx = 0;
    QString text =
        "Δt = " +
        SuWidgetsHelpers::formatQuantity(
          this->samp2t(this->hSelEnd) - this->samp2t(this->hSelStart),
          4,
          "s");

    pen.setStyle(Qt::DashLine);
    pen.setColor(this->text);

    p.setPen(pen);
    p.drawLine(xStart, 0, xStart, this->geometry.height() - 1);
    p.drawLine(xEnd,   0, xEnd, this->geometry.height() - 1);


#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    tw = metrics.horizontalAdvance(text);
#else
    tw = metrics.width(text);
#endif // QT_VERSION_CHECK

    rect.setRect(
          px + metrics.height() / 2,
          ypx,
          tw,
          metrics.height());
    p.setOpacity(1);
    p.drawText(rect, Qt::AlignHCenter | Qt::AlignBottom, text);

  }
}

void
Waveform::draw()
{
  if (!this->size().isValid())
    return;

  // This is not valid either
  if (this->size().width() * this->size().height() < 1)
    return;

  QRect rect(0, 0, this->size().width(), this->size().height());

  if (this->geometry != this->size()) {
    m_view.setGeometry(this->size().width(), this->size().height());

    this->geometry = this->size();
    if (!this->haveGeometry) {
      this->haveGeometry = true;
      this->zoomVerticalReset();
      this->zoomHorizontalReset();
    }

    this->axesPixmap    = QPixmap(rect.size());
    this->contentPixmap = QPixmap(rect.size());
    this->waveform      = QImage(this->geometry, QImage::Format_ARGB32);

    this->recalculateDisplayData();
    this->selUpdated = false;
    this->axesDrawn  = false;
    this->waveDrawn  = false;
  } else if (!this->isComplete() && !this->enableFeedback) {
    // If we were called because we need to redraw something, but we
    // explicitly disabled feedback and there is a previous waveform
    // image that we can reuse, we wait for later to redraw everything
    return;
  }

  //
  // Dirty means that something has changed and we must redraw the whole
  // content pixmap, layer by layer
  //
  if (this->somethingDirty()) {
    if (!this->axesDrawn) {
      this->drawAxes();
      this->axesDrawn = true;
      this->waveDrawn = false;
    }

    if (!this->waveDrawn) {
      this->drawWave();
      this->waveDrawn = true;
    }

    // Clear current content pixmap
    this->contentPixmap.fill(this->background);
    QPainter p(&this->contentPixmap);

    // Stack layers in order. First, stack axes
    p.drawPixmap(rect, this->axesPixmap);

    // Stack wave
    p.drawImage(0, 0, this->waveform);

    // Selection present, overlay
    if (this->hSelection) {
      this->overlaySelection(p);
      this->overlaySelectionMarkes(p);
    }

    this->selUpdated = true;
    p.end();
  }
}

void
Waveform::paint()
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
  m_view.borrowTree(other->m_view);
}

void
Waveform::setData(
    const std::vector<SUCOMPLEX> *data,
    bool keepView,
    bool flush)
{
  bool   appending  = data != nullptr && data == m_data.loanedBuffer();
  qint64 prevLength = SCAST(qint64, m_view.getLength());
  qint64 newLength  = data == nullptr ? 0 : static_cast<qint64>(data->size());
  qint64 extra      = newLength - prevLength;

  this->askedToKeepView = keepView;

  if (appending) {
    // The current wavebuffer holds a reference to the same data vector,
    // we only inform the view to recalculate the new samples
    if (flush) {
      m_view.setBuffer(data);
    } else if (extra > 0) {
      m_view.refreshBuffer(data);
    }

  } else {
    if (data != nullptr)
      m_data = WaveBuffer(&m_view, data);
    else
      m_data = WaveBuffer(&m_view);
  }
}

void
Waveform::setData(
    const SUCOMPLEX *data,
    size_t size,
    bool keepView,
    bool flush,
    bool appending)
{
  qint64 prevLength = SCAST(qint64, m_view.getLength());
  qint64 extra      = size - prevLength;

  this->askedToKeepView = keepView;

  if (appending) {
    // The current wavebuffer holds a reference to the same data vector,
    // we only inform the view to recalculate the new samples
    if (flush) {
      m_view.setBuffer(data, size);
    } else if (extra > 0) {
      m_view.refreshBuffer(data, size);
    }

  } else {
    if (data != nullptr)
      m_data = WaveBuffer(&m_view, data, size);
    else
      m_data = WaveBuffer(&m_view);
  }
}

void
Waveform::setRealComponent(bool real)
{
  m_view.setRealComponent(real);
  this->fitToEnvelope();
  this->invalidate();
}

void
Waveform::setShowEnvelope(bool show)
{
  m_view.setShowEnvelope(show);
  this->waveDrawn = false;
  this->axesDrawn = false;
  this->invalidate();
}

void
Waveform::setShowPhase(bool show)
{
  m_view.setShowPhase(show);
  if (m_view.isEnvelopeVisible()) {
    this->waveDrawn = false;
    this->axesDrawn = false;
    this->invalidate();
  }
}

void
Waveform::setShowPhaseDiff(bool show)
{
  m_view.setShowPhaseDiff(show);

  if (m_view.isEnvelopeVisible()) {
    this->waveDrawn = false;
    this->axesDrawn = false;
    this->invalidate();
  }
}

void
Waveform::setPhaseDiffOrigin(unsigned origin)
{
  m_view.setPhaseDiffOrigin(origin);

  if (m_view.isEnvelopeVisible()
      && m_view.isPhaseEnabled()
      && m_view.isPhaseDiffEnabled()) {
    this->waveDrawn = false;
    this->axesDrawn = false;
    this->invalidate();
  }
}

void
Waveform::setPhaseDiffContrast(qreal contrast)
{
  m_view.setPhaseDiffContrast(contrast);

  if (m_view.isEnvelopeVisible()
      && m_view.isPhaseEnabled()
      && m_view.isPhaseDiffEnabled()) {
    this->waveDrawn = false;
    this->axesDrawn = false;
    this->invalidate();
  }
}

void
Waveform::setShowWaveform(bool show)
{
  m_view.setShowWaveform(show);

  this->waveDrawn = false;
  this->axesDrawn = false;
  this->invalidate();
}

void
Waveform::triggerMouseMoveHere()
{
  QPoint pos = QCursor::pos();

  QMouseEvent *event = new QMouseEvent(
        QEvent::Type::MouseMove,
        QPointF(this->mapFromGlobal(pos)),
        QPointF(pos),
        Qt::NoButton,
        QApplication::mouseButtons(),
        QApplication::keyboardModifiers());

  mouseMoveEvent(event);

  delete event;
}

void
Waveform::refreshData()
{
  qint64 currSpan = m_view.getViewSampleInterval();
  qint64 lastSample = this->getDataLength() - 1;

  this->askedToKeepView = true;
  m_data.rebuildViews();

  if (this->autoScroll && this->getSampleEnd() <= lastSample) {
    m_view.setHorizontalZoom(lastSample - currSpan, lastSample);

    if (this->hSelDragging)
      triggerMouseMoveHere();
  }

  this->waveDrawn = false;

  this->recalculateDisplayData();

  if (this->autoFitToEnvelope)
    this->fitToEnvelope();
  else
    this->axesDrawn = false;
}

Waveform::Waveform(QWidget *parent) :
  ThrottleableWidget(parent),
  m_data(&m_view)
{
  std::vector<QColor> colorTable;

  m_view.setSampleRate(1024000);

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

  m_view.setPalette(colorTable.data());
  m_view.setForeground(this->foreground);

  connect(
        &m_view,
        SIGNAL(ready()),
        this,
        SLOT(onWaveViewChanges()));

  connect(
        &m_view,
        SIGNAL(progress()),
        this,
        SLOT(onWaveViewChanges()));

  this->setMouseTracking(true);
  this->invalidate();
}

void
Waveform::onWaveViewChanges()
{
  if (!this->isComplete() && !this->enableFeedback)
    return;

  this->waveDrawn = false;
  this->axesDrawn = false;

  if (!this->askedToKeepView) {
    this->resetSelection();

    if (this->autoFitToEnvelope)
      this->fitToEnvelope();
    else
      this->zoomVerticalReset();

    this->zoomHorizontalReset();
  } else {
    this->axesDrawn = false;
    this->selUpdated = false;
  }

  this->invalidate();
  emit waveViewChanged();
}
