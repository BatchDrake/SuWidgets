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
  assert(isLoan() || m_buffer == &m_ownBuffer);

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

  m_hDivSamples = divLen * m_view.getSampleRate();

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
  m_vDivUnits = divLen;
}

void
Waveform::safeCancel()
{
  m_view.safeCancel();
}

void
Waveform::zoomHorizontalReset()
{
  if (m_haveGeometry) {
    qint64 length = SCAST(qint64, m_data.length());

    if (length > 0)
      zoomHorizontal(SCAST(qint64, 0), length - 1);
    else if (getSampleRate() > 0)
      zoomHorizontal(
          SCAST(qint64, 0),
          SCAST(qint64, getSampleRate()));
    else
      zoomHorizontal(SCAST(qint64, 0), SCAST(qint64, 0));
  }
}

void
Waveform::zoomHorizontal(qint64 x, qreal amount)
{
  qreal newRange;
  qreal fixedSamp;
  qreal relPoint = static_cast<qreal>(x - m_valueTextWidth) / (m_view.width());

  //
  // This means that position at x remains the same, while the others shrink
  // or stretch accordingly
  //

  fixedSamp = std::round(px2samp(x));
  newRange  = std::ceil(amount * m_view.getViewSampleInterval());

  zoomHorizontal(
        static_cast<qint64>(std::round(fixedSamp - relPoint * newRange)),
        static_cast<qint64>(std::round(fixedSamp + (1.0 - relPoint) * newRange)));
}

void
Waveform::zoomHorizontal(qreal tStart, qreal tEnd)
{
  zoomHorizontal(
        static_cast<qint64>(t2samp(tStart)),
        static_cast<qint64>(t2samp(tEnd)));
}

void
Waveform::zoomHorizontal(qint64 start, qint64 end)
{
  qint64 currStart = getSampleStart();
  qint64 currEnd   = getSampleEnd();

  if (start != currStart || end != currEnd) {
    m_view.setHorizontalZoom(start, end);
    if (m_hSelection)
      m_selUpdated = false;
    m_axesDrawn = false;
    recalculateDisplayData();
    emit horizontalRangeChanged(start, end);
  }
}

void
Waveform::saveHorizontal()
{
  m_savedStart = getSampleStart();
  m_savedEnd   = getSampleEnd();
}

void
Waveform::scrollHorizontal(qint64 orig, qint64 to)
{
  scrollHorizontal(to - orig);
}

void
Waveform::scrollHorizontal(qint64 delta)
{
  zoomHorizontal(
        m_savedStart - static_cast<qint64>(delta * getSamplesPerPixel()),
        m_savedEnd   - static_cast<qint64>(delta * getSamplesPerPixel()));
}

void
Waveform::selectHorizontal(qreal orig, qreal to)
{
  m_hSelection = true;

  if (orig < to) {
    m_hSelStart = orig;
    m_hSelEnd   = to;
  } else if (to < orig) {
    m_hSelStart = to;
    m_hSelEnd   = orig;
  } else {
    m_hSelection = false;
  }

  m_selUpdated = false;

  emit horizontalSelectionChanged(m_hSelStart, m_hSelEnd);
}

bool
Waveform::getHorizontalSelectionPresent() const
{
  return getDataLength() > 0 && m_hSelection;
}

qreal
Waveform::getHorizontalSelectionStart() const
{
  if (!getHorizontalSelectionPresent())
    return .0;
  else
    return qBound(.0, m_hSelStart, SCAST(qreal, getDataLength() - 1));
}

qreal
Waveform::getHorizontalSelectionEnd() const
{
  if (!getHorizontalSelectionPresent())
    return .0;
  else
    return qBound(.0, m_hSelEnd, SCAST(qreal, getDataLength() - 1));
}

void
Waveform::setAutoScroll(bool value)
{
  m_autoScroll = value;
  refreshData();
}

void
Waveform::zoomVerticalReset()
{
  zoomVertical(-1., 1.);
}

void
Waveform::zoomVertical(qint64 y, qreal amount)
{
  qreal val = px2value(y);

  zoomVertical(
        (getMin() - val) * amount + val,
        (getMax() - val) * amount + val);
}

void
Waveform::zoomVertical(qreal min, qreal max)
{
  m_view.setVerticalZoom(min, max);
  m_axesDrawn = false;

  recalculateDisplayData();
  emit verticalRangeChanged(min, max);
}

void
Waveform::saveVertical()
{
  m_savedMin = getMin();
  m_savedMax = getMax();
}

void
Waveform::scrollVertical(qint64 orig, qint64 to)
{
  scrollVertical(to - orig);
}

void
Waveform::scrollVertical(qint64 delta)
{
  zoomVertical(
        m_savedMin + delta * getUnitsPerPx(),
        m_savedMax + delta * getUnitsPerPx());
}

void
Waveform::selectVertical(qint64 orig, qint64 to)
{
  m_vSelection = true;

  if (orig < to) {
    m_vSelStart = orig;
    m_vSelEnd   = to;
  } else if (to < orig) {
    m_vSelStart = to;
    m_vSelEnd   = orig;
  } else {
    m_vSelection = false;
  }

  m_selUpdated = false;

  emit verticalSelectionChanged(m_vSelStart, m_vSelEnd);
}

bool
Waveform::getVerticalSelectionPresent() const
{
  return m_vSelection;
}

qreal
Waveform::getVerticalSelectionStart() const
{
  return m_vSelStart;
}

qreal
Waveform::getVerticalSelectionEnd() const
{
  return m_vSelEnd;
}

void
Waveform::fitToEnvelope()
{
  qreal envelope = m_view.getEnvelope();

  if (envelope > 0)
    zoomVertical(-envelope, envelope);
}

void
Waveform::setAutoFitToEnvelope(bool autoFit)
{
  m_autoFitToEnvelope = autoFit;
}


void
Waveform::resetSelection()
{
  m_hSelection = m_vSelection = false;
  m_selUpdated = false;
}

void
Waveform::setPeriodicSelection(bool val)
{
  m_periodicSelection = val;
  m_selUpdated = false;
}

///////////////////////////////// Events ///////////////////////////////////////
void
Waveform::mouseMoveEvent(QMouseEvent *event)
{
  // Emit mouseMove
  m_haveCursor = true;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  m_currMouseX = event->position().x();
#else
  currMouseX = event->x();
#endif

  if (m_frequencyDragging)
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    scrollHorizontal(m_clickX, event->position().x());
#else
    scrollHorizontal(clickX, event->x());
#endif
  else if (m_valueDragging)
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    scrollVertical(m_clickY, event->position().y());
#else
    scrollVertical(clickY, event->y());
#endif
  else if (m_hSelDragging)
    selectHorizontal(
          static_cast<qint64>(px2samp(samp2px(m_clickSample))),
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
          static_cast<qint64>(px2samp(event->position().x())));
#else
          static_cast<qint64>(px2samp(event->x())));
#endif

  emit hoverTime(px2t(m_currMouseX));
  invalidate();
}

void
Waveform::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::RightButton) {
    zoomHorizontalReset();
    invalidateHard();
  } else {
    // Save selection
    saveHorizontal();
    saveVertical();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_clickX = event->position().x();
    m_clickY = event->position().y();
#else
    clickX = event->x();
    clickY = event->y();
#endif

    m_clickSample = px2samp(m_clickX);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (event->button() == Qt::MiddleButton
#else
    if (event->button() == Qt::MidButton
#endif // QT_VERSION
        || m_clickY >= m_geometry.height() - m_frequencyTextHeight)
      m_frequencyDragging = true;
    else if (m_clickX < m_valueTextWidth)
      m_valueDragging = !m_autoFitToEnvelope && true;
    else
      m_hSelDragging = true;
  }
}

void
Waveform::mouseReleaseEvent(QMouseEvent *event)
{
  mouseMoveEvent(event);
  m_frequencyDragging = false;
  m_valueDragging     = false;
  m_hSelDragging      = false;
}

void
Waveform::mouseDoubleClickEvent(QMouseEvent *event)
{
  int x = event->pos().x();
  int y = event->pos().y();
  qreal t = px2samp(x);
  qreal v = px2value(y);

  selectHorizontal(0, 0);

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

  if (delta >= -WAVEFORM_DELTA_LIMIT && delta <= WAVEFORM_DELTA_LIMIT) {
    qreal amount = std::pow(
          static_cast<qreal>(1.1),
          static_cast<qreal>(-delta / 120.));
    if (x < m_valueTextWidth) {
      if (!m_autoFitToEnvelope)
        zoomVertical(static_cast<qint64>(y), amount);
    } else {
      zoomHorizontal(static_cast<qint64>(x), amount);
    }

    invalidate();
  }
}

void
Waveform::leaveEvent(QEvent *)
{
  m_haveCursor = false;
  invalidate();
}

//////////////////////////////// Drawing methods ///////////////////////////////////
void
Waveform::overlaySelection(QPainter &p)
{
  if (m_hSelection) {
    // qreal selLen = hSelEnd - hSelStart;
    int xStart = SCAST(int, samp2px(m_hSelStart));
    int xEnd   = SCAST(int, samp2px(m_hSelEnd));

    QRect rect1(
          0,
          0,
          xStart,
          m_geometry.height());

    QRect rect2(
          xEnd,
          0,
          m_geometry.width() - xEnd,
          m_geometry.height());

    if (rect1.x() < m_valueTextWidth)
      rect1.setX(m_valueTextWidth);

    if (rect1.right() >= m_geometry.width())
      rect1.setRight(m_geometry.width() - 1);

    if (rect2.x() < m_valueTextWidth)
      rect2.setX(m_valueTextWidth);

    if (rect2.right() >= m_geometry.width())
      rect2.setRight(m_geometry.width() - 1);


    p.save();
    p.setOpacity(.5);
    p.fillRect(rect1, m_selection);
    p.fillRect(rect2, m_selection);
    p.restore();
  }
}

void
Waveform::paintTriangle(
    QPainter &p,
    int x,
    int y,
    int orientation,
    QColor const &color,
    int side)
{
  QPainterPath path;

  p.save();
  p.translate(x, y);

  if (orientation > 0)
    p.rotate(orientation * 90);

  path.moveTo(2 * side,  0);
  path.lineTo( 0,    -side);
  path.lineTo( 0,    +side);
  path.lineTo(2 * side,  0);

  p.setPen(Qt::NoPen);
  p.fillPath(path, QBrush(color));

  p.restore();
}
void
Waveform::overlayPoints(QPainter &p)
{
  if (m_pointMap.size() > 0) {
    QFont font;
    QFontMetrics metrics(font);
    QRect rect;
    int orientation;

    QMap<qreal, WavePoint>::iterator begin =
        m_pointMap.lowerBound(samp2t(getSampleStart()));
    QMap<qreal, WavePoint>::iterator end   =
        m_pointMap.upperBound(samp2t(getSampleEnd()));

    for (auto m = begin; m != end; ++m) {
      int tw;
      qint64 xpx = SCAST(qint64, t2px(m->t));

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
      tw = metrics.horizontalAdvance(m->string);
#else
      tw = metrics.width(m->string);
#endif // QT_VERSION_CHECK

      if (xpx >= 0 && xpx < m_geometry.width() - tw / 2) {
        qreal y = m_view.isRealComponent()
            ? SU_C_REAL(m->point)
            : SU_C_IMAG(m->point);
        int preferredYpx = value2px(y);
        int ypx = qBound(0, preferredYpx, m_geometry.height() - metrics.height());
        int angle;
        int extra;
        int gap;

        if (preferredYpx < ypx) {
          orientation = 3;
          angle       = 45;
          extra       = 10;
          gap         = 10;
        } else if (preferredYpx > ypx) {
          angle       = -45;
          orientation = 1;
          extra       = -10;
          gap         = 10;
        } else {
          angle       = m->angle;
          orientation = 0;
          extra       = 0;
          gap         = 0;
        }

        p.save();
          if (orientation == 0) {
            p.setBrush(m->color);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPoint(xpx, ypx), WAVEFORM_POINT_RADIUS, WAVEFORM_POINT_RADIUS);
          } else {
            paintTriangle(p, xpx, ypx + extra, orientation, m->color);
          }
        p.restore();

        if (m->string.size() > 0) {
          p.save();
            p.setPen(m->color);
            p.translate(xpx, ypx);

            if (angle != 0)
              p.rotate(angle);

            rect.setRect(
                WAVEFORM_POINT_RADIUS + WAVEFORM_POINT_SPACING + gap,
                -metrics.height() / 2,
                tw,
                metrics.height());
            p.setOpacity(1);
            p.drawText(rect, Qt::AlignHCenter | Qt::AlignBottom,m->string);
          p.restore();
        }
      }
    }
  }
}

void
Waveform::overlayMarkers(QPainter &p)
{
  if (m_markerList.size() > 0) {
    QFont font;
    QFontMetrics metrics(font);
    QPen pen(m_text);
    QRect rect;

    p.setPen(pen);

    for (auto m = m_markerList.begin();
         m != m_markerList.end();
         ++m)
    {
      int tw;
      qint64 px = SCAST(qint64, samp2px(m->x));

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
      tw = metrics.horizontalAdvance(m->string);
#else
      tw = metrics.width(m->string);
#endif // QT_VERSION_CHECK

      if (px >= 0 && px < m_geometry.width() - tw / 2) {
        qreal y = m->x < getDataLength()
            ? cast(getData()[m->x])
            : 0;
        int ypx = value2px(y) +
            (m->below ? 2 : - metrics.height() - 2);

        ypx = qBound(0, ypx, m_geometry.height() - metrics.height());

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
  if (m_vCursorList.size() > 0) {
    int width = p.device()->width();
    QFont font;
    QFontMetrics metrics(font);
    QPen pen;
    int x = m_valueTextWidth;

    p.save();

    pen.setStyle(Qt::DashLine);
    pen.setWidth(1);

    p.setOpacity(1);

    for (auto c = m_vCursorList.begin();
         c != m_vCursorList.end();
         ++c) {
      QPainterPath path;
      int y = SCAST(int, value2px(cast(c->level)));

      paintTriangle(p, x, y, 0, c->color);

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
  if (m_aCursorList.size() > 0) {
    QFont font;
    QFontMetrics metrics(font);
    int x = m_valueTextWidth;
    int width = p.device()->width();

    p.save();

    p.setOpacity(1);

    for (auto a = m_aCursorList.begin(); a != m_aCursorList.end(); ++a) {
      QPainterPath path;
      QPen pen;
      int y1 = SCAST(int, value2px(+a->amplitude));
      int y2 = SCAST(int, value2px(-a->amplitude));

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
  m_waveform.fill(Qt::transparent);
  QPainter p(&m_waveform);

  overlayACursors(p);
  m_view.drawWave(p);
  overlayMarkers(p);
  overlayVCursors(p);
  overlayPoints(p);
  p.end();
}

static inline int
estimateTextWidth(QFontMetrics &metrics, QString label)
{
  int tw;
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        tw = metrics.horizontalAdvance(label);
#else
        tw = metrics.width(label);
#endif // QT_VERSION_CHECK
  return tw;
}

void
Waveform::drawVerticalAxes()
{
  QFont font;
  QPainter p(&m_axesPixmap);
  QFontMetrics metrics(font);
  QRect rect;
  QPen pen(m_axes);
  qreal deltaT = m_view.getDeltaT();
  int axis;
  int px;
  bool unixTime = m_horizontalUnits == "unix";
  int previousLabel = -1;

  pen.setStyle(Qt::DotLine);
  p.setPen(pen);
  p.setFont(font);

  m_frequencyTextHeight = metrics.height();

  if (m_hDivSamples > 0) {
    qreal rem = m_oX -
        m_hDivSamples * std::floor(m_oX / m_hDivSamples);

    // Draw axes
    axis = static_cast<int>(std::floor(getSampleStart() / m_hDivSamples));

    while (axis * m_hDivSamples <= getSampleEnd() + rem) {
      px = static_cast<int>(samp2px(axis * m_hDivSamples - rem));

      if (px > 0)
        p.drawLine(px, 0, px, m_geometry.height() - 1);
      ++axis;
    }

    // Draw labels
    p.setPen(m_text);
    axis = static_cast<int>(std::floor(getSampleStart() / m_hDivSamples));
    while (axis * m_hDivSamples <= getSampleEnd() + rem) {
      px = static_cast<int>(samp2px(axis * m_hDivSamples - rem));

      if (px > 0) {
        QString label;
        int tw;

        if (unixTime)
          label = SuWidgetsHelpers::formatQuantity(
                (m_oX + axis * m_hDivSamples - rem) * deltaT + m_view.samp2t(0),
                0,
                m_horizontalUnits);
        else
          label = SuWidgetsHelpers::formatQuantityFromDelta(
                (m_oX + axis * m_hDivSamples - rem) * deltaT,
                m_hDivSamples * deltaT,
                m_horizontalUnits);

        tw = estimateTextWidth(metrics, label);

        if (previousLabel == -1 || previousLabel < px - tw / 2) {
          rect.setRect(
                px - tw / 2,
                m_geometry.height() - m_frequencyTextHeight,
                tw,
                m_frequencyTextHeight);
          p.drawText(rect, Qt::AlignHCenter | Qt::AlignBottom, label);
          previousLabel = px + tw / 2;
        }
      }
      ++axis;
    }
  }
  p.end();
}

int
Waveform::calcWaveViewWidth() const
{
  int viewWidth = width() - m_valueTextWidth;
  if (viewWidth < m_valueTextWidth)
    viewWidth = m_valueTextWidth;

  return viewWidth;
}

void
Waveform::drawHorizontalAxes()
{
  QFont font;
  QPainter p(&m_axesPixmap);
  QFontMetrics metrics(font);
  QRect rect;
  QPen pen(m_axes);
  int axis;
  int px;

  p.setPen(pen);
  p.setFont(font);

  if (m_vDivUnits > 0) {
    axis = static_cast<int>(std::floor(getMin() / m_vDivUnits));

    while (axis * m_vDivUnits <= getMax()) {
      pen.setStyle(axis == 0 ? Qt::SolidLine : Qt::DotLine);
      p.setPen(pen);
      px = static_cast<int>(value2px(axis * m_vDivUnits));

      if (px > 0)
        p.drawLine(0, px, m_geometry.width() - 1, px);
      ++axis;
    }

    p.setPen(m_text);
    axis = static_cast<int>(std::floor(getMin() / m_vDivUnits));
    while (axis * m_vDivUnits <= getMax()) {
      px = static_cast<int>(value2px(axis * m_vDivUnits));

      if (px > 0) {
        QString label;
        int tw;

        label = SuWidgetsHelpers::formatQuantityFromDelta(
              axis * m_vDivUnits,
              m_vDivUnits,
              m_verticalUnits);

        tw = estimateTextWidth(metrics, label);

        rect.setRect(
              0,
              px - metrics.height() / 2,
              tw,
              metrics.height());

        p.fillRect(rect, m_background);
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
  m_axesPixmap.fill(Qt::transparent);
  drawHorizontalAxes();
  drawVerticalAxes();
}

void
Waveform::overlaySelectionMarkes(QPainter &p)
{
  int xStart = SCAST(int, samp2px(m_hSelStart));
  int xEnd   = SCAST(int, samp2px(m_hSelEnd));

  if (m_periodicSelection) {
    qreal selLen   = m_hSelEnd - m_hSelStart;
    qreal deltaDiv = selLen / static_cast<qreal>(m_divsPerSelection);
    bool manyLines = deltaDiv <= getSamplesPerPixel();

    if (manyLines) {
      QRect rect(
            xStart,
            0,
            xEnd - xStart,
            m_geometry.height() - m_frequencyTextHeight);

      if (rect.x() < m_valueTextWidth)
        rect.setX(m_valueTextWidth);

      if (rect.right() >= m_geometry.width())
        rect.setRight(m_geometry.width() - 1);

      p.save();
      p.setOpacity(.5);
      p.fillRect(rect, m_subSelection);
      p.restore();
    } else {
      QPen pen;
      pen.setStyle(Qt::DashLine);
      pen.setColor(m_subSelection);

      p.setOpacity(1);

      for (int i = 0; i <= m_divsPerSelection; ++i) {
        qreal divSample = i * deltaDiv + m_hSelStart;
        int px = static_cast<int>(samp2px(divSample));

        p.setPen(pen);

        if (px > m_valueTextWidth && px < m_geometry.width())
          p.drawLine(
                px,
                0,
                px,
                m_geometry.height() - m_frequencyTextHeight);
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
        "Δ" + m_horizontalAxis + " = " +
        SuWidgetsHelpers::formatQuantity(
          samp2t(m_hSelEnd) - samp2t(m_hSelStart),
          4,
          getHorizontalUnits());

    pen.setStyle(Qt::DashLine);
    pen.setColor(m_text);

    p.setPen(pen);
    p.drawLine(xStart, 0, xStart, m_geometry.height() - 1);
    p.drawLine(xEnd,   0, xEnd, m_geometry.height() - 1);


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
  if (!size().isValid())
    return;

  // This is not valid either
  if (size().width() * size().height() < 1)
    return;

  QRect rect(0, 0, size().width(), size().height());

  if (m_geometry != size()) {
    if (m_valueTextWidth == 0) {
      QFont font;
      QFontMetrics metrics(font);
      m_valueTextWidth = estimateTextWidth(metrics, "+00.00 dB");
    }

    m_geometry = size();
    if (m_view.width() != calcWaveViewWidth())
      m_view.setGeometry(calcWaveViewWidth(), height());

    if (!m_haveGeometry) {
      m_haveGeometry = true;
      if (m_autoFitToEnvelope)
        zoomVerticalReset();
      zoomHorizontalReset();
    }

    m_axesPixmap    = QPixmap(rect.size());
    m_contentPixmap = QPixmap(rect.size());
    m_waveform      = QImage(
          m_view.width(),
          m_view.height(),
          QImage::Format_ARGB32);

    recalculateDisplayData();
    m_selUpdated = false;
    m_axesDrawn  = false;
    m_waveDrawn  = false;
  } else if (!isComplete() && !m_enableFeedback) {
    // If we were called because we need to redraw something, but we
    // explicitly disabled feedback and there is a previous waveform
    // image that we can reuse, we wait for later to redraw everything
    return;
  }

  //
  // Dirty means that something has changed and we must redraw the whole
  // content pixmap, layer by layer
  //
  if (somethingDirty()) {
    if (!m_axesDrawn) {
      drawAxes();
      m_axesDrawn = true;
      m_waveDrawn = false;
    }

    if (!m_waveDrawn) {
      drawWave();
      m_waveDrawn = true;
    }

    // Clear current content pixmap
    m_contentPixmap.fill(m_background);
    QPainter p(&m_contentPixmap);

    // Stack layers in order. First, stack axes
    p.drawPixmap(rect, m_axesPixmap);

    // Stack wave
    p.drawImage(m_valueTextWidth, 0, m_waveform);

    // Selection present, overlay
    if (m_hSelection) {
      overlaySelection(p);
      overlaySelectionMarkes(p);
    }

    m_selUpdated = true;
    p.end();
  }
}

void
Waveform::paint()
{
  QPainter painter(this);
  painter.drawPixmap(0, 0, m_contentPixmap);

  if (m_haveCursor) {
    painter.setPen(m_axes);
    painter.drawLine(
          m_currMouseX,
          0,
          m_currMouseX,
          m_geometry.height() - 1);
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

  m_askedToKeepView = keepView;

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

  m_askedToKeepView = keepView;

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
  fitToEnvelope();
  invalidate();
}

void
Waveform::setShowEnvelope(bool show)
{
  m_view.setShowEnvelope(show);
  m_waveDrawn = false;
  m_axesDrawn = false;
  invalidate();
}

void
Waveform::setShowPhase(bool show)
{
  m_view.setShowPhase(show);
  if (m_view.isEnvelopeVisible()) {
    m_waveDrawn = false;
    m_axesDrawn = false;
    invalidate();
  }
}

void
Waveform::setShowPhaseDiff(bool show)
{
  m_view.setShowPhaseDiff(show);

  if (m_view.isEnvelopeVisible()) {
    m_waveDrawn = false;
    m_axesDrawn = false;
    invalidate();
  }
}

void
Waveform::setPhaseDiffOrigin(unsigned origin)
{
  m_view.setPhaseDiffOrigin(origin);

  if (m_view.isEnvelopeVisible()
      && m_view.isPhaseEnabled()
      && m_view.isPhaseDiffEnabled()) {
    m_waveDrawn = false;
    m_axesDrawn = false;
    invalidate();
  }
}

void
Waveform::setPhaseDiffContrast(qreal contrast)
{
  m_view.setPhaseDiffContrast(contrast);

  if (m_view.isEnvelopeVisible()
      && m_view.isPhaseEnabled()
      && m_view.isPhaseDiffEnabled()) {
    m_waveDrawn = false;
    m_axesDrawn = false;
    invalidate();
  }
}

void
Waveform::setShowWaveform(bool show)
{
  m_view.setShowWaveform(show);

  m_waveDrawn = false;
  m_axesDrawn = false;
  invalidate();
}

void
Waveform::triggerMouseMoveHere()
{
  QPoint pos = QCursor::pos();

  QMouseEvent *event = new QMouseEvent(
        QEvent::Type::MouseMove,
        QPointF(mapFromGlobal(pos)),
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
  qint64 lastSample = getDataLength() - 1;

  m_askedToKeepView = true;
  m_data.rebuildViews();

  if (m_autoScroll && getSampleEnd() <= lastSample) {
    m_view.setHorizontalZoom(lastSample - currSpan, lastSample);

    if (m_hSelDragging)
      triggerMouseMoveHere();
  }

  m_waveDrawn = false;

  recalculateDisplayData();

  if (m_autoFitToEnvelope)
    fitToEnvelope();
  else
    m_axesDrawn = false;
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

  m_background   = WAVEFORM_DEFAULT_BACKGROUND_COLOR;
  m_foreground   = WAVEFORM_DEFAULT_FOREGROUND_COLOR;
  m_axes         = WAVEFORM_DEFAULT_AXES_COLOR;
  m_text         = WAVEFORM_DEFAULT_TEXT_COLOR;
  m_selection    = WAVEFORM_DEFAULT_SELECTION_COLOR;
  m_subSelection = WAVEFORM_DEFAULT_SUBSEL_COLOR;
  m_envelope     = WAVEFORM_DEFAULT_ENVELOPE_COLOR;

  m_view.setPalette(colorTable.data());
  m_view.setForeground(m_foreground);

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

  setMouseTracking(true);
  invalidate();
}

Waveform::~Waveform()
{
}

void
Waveform::onWaveViewChanges()
{
  if (!isComplete() && !m_enableFeedback)
    return;

  m_waveDrawn = false;
  m_axesDrawn = false;

  if (!m_askedToKeepView) {
    resetSelection();

    if (m_autoFitToEnvelope)
      fitToEnvelope();
    else
      zoomVerticalReset();

    zoomHorizontalReset();
  } else {
    m_axesDrawn = false;
    m_selUpdated = false;
  }

  invalidate();
  emit waveViewChanged();
}
