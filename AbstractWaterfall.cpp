/* -*- c++ -*- */
/* + + +   This Software is released under the "Simplified BSD License"  + + +
 * Copyright 2010 Moe Wheatley. All rights reserved.
 * Copyright 2011-2013 Alexandru Csete OZ9AEC
 * Copyright 2018 Gonzalo José Carracedo Carballal - Minimal modifications for integration
 * Copyright 2023-2024 Sultan Qasim Khan - Abstract class for OpenGL and QPainter waterfalls
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY Moe Wheatley ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Moe Wheatley OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Moe Wheatley.
 */
#include <cmath>
#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QFont>
#include <QPainter>
#include <QtGlobal>
#include <QToolTip>
#include <QDebug>

#include "AbstractWaterfall.h"
#include "SuWidgetsHelpers.h"

// Comment out to enable plotter debug messages
//#define PLOTTER_DEBUG

#define STATUS_TIP \
  "Click, drag or scroll on spectrum to tune. " \
  "Drag and scroll X and Y axes for pan and zoom. " \
  "Drag filter edges to adjust filter."

///////////////////////////// AbstractWaterfall ////////////////////////////////////////
AbstractWaterfall::AbstractWaterfall(QWidget *parent) : QOpenGLWidget(parent)
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setFocusPolicy(Qt::StrongFocus);
  setAttribute(Qt::WA_PaintOnScreen,false);
  setAutoFillBackground(false);
  setAttribute(Qt::WA_OpaquePaintEvent, false);
  setAttribute(Qt::WA_NoSystemBackground, true);
  setMouseTracking(true);

  setTooltipsEnabled(false);
  setStatusTip(tr(STATUS_TIP));

  m_PeakHoldActive = false;
  m_PeakHoldValid = false;

  m_FftCenter = 0;
  m_CenterFreq = 144500000;
  m_DemodCenterFreq = 144500000;
  m_DemodHiCutFreq = 5000;
  m_DemodLowCutFreq = -5000;

  m_FLowCmin = -25000;
  m_FLowCmax = -1000;
  m_FHiCmin = 1000;
  m_FHiCmax = 25000;
  m_symetric = true;

  m_ClickResolution = 100;
  m_FilterClickResolution = 100;
  m_CursorCaptureDelta = CUR_CUT_DELTA;

  m_FilterBoxEnabled = true;
  m_CenterLineEnabled = true;
  m_BookmarksEnabled = true;
  m_Locked = false;
  m_freqDragLocked = false;
  m_Span = 96000;
  m_SampleFreq = 96000;

  m_HorDivs = 12;
  m_VerDivs = 6;
  m_PandMaxdB = m_WfMaxdB = 0.f;
  m_PandMindB = m_WfMindB = -150.f;

  m_CumWheelDelta = 0;
  m_FreqUnits = 1000000;
  m_CursorCaptured = NOCAP;
  m_Running = false;
  m_DrawOverlay = true;
  m_2DPixmap = QPixmap(0,0);
  m_OverlayPixmap = QPixmap(0,0);
  m_Size = QSize(0,0);
  m_SpectrumPlotHeight = 0;
  m_WaterfallHeight = 0;
  m_GrabPosition = 0;
  m_Percent2DScreen = 30;	//percent of screen used for 2D display
  m_VdivDelta = 30;
  m_HdivDelta = 70;

  m_ZeroPoint = 0;
  m_dBPerUnit = 1;

  m_FreqDigits = 3;

  m_Peaks = QMap<int,int>();
  setPeakDetection(false, 2);
  m_PeakHoldValid = false;

  setFftPlotColor(QColor(0xFF,0xFF,0xFF,0xFF));
  setFftBgColor(QColor(PLOTTER_BGD_COLOR));
  setFftAxesColor(QColor(PLOTTER_GRID_COLOR));
  setFilterBoxColor(QColor(PLOTTER_FILTER_BOX_COLOR));
  setTimeStampColor(QColor(0xFF,0xFF,0xFF,0xFF));

  setFftFill(false);

  // always update waterfall
  tlast_wf_ms = 0;
  msec_per_wfline = 0;
  wf_span = 0;
  fft_rate = 15;

  m_fftData = nullptr;
  m_fftDataSize = 0;

  m_infoTextColor = m_FftTextColor;
}

AbstractWaterfall::~AbstractWaterfall()
{
}

QSize AbstractWaterfall::minimumSizeHint() const
{
  return QSize(50, 50);
}

QSize AbstractWaterfall::sizeHint() const
{
  return QSize(180, 180);
}

void AbstractWaterfall::mouseMoveEvent(QMouseEvent* event)
{
  QPoint pt = event->pos();

  /* mouse enter / mouse leave events */
  if (m_OverlayPixmap.rect().contains(pt))
  {
    //is in Overlay bitmap region
    if (event->buttons() == Qt::NoButton)
    {
      bool onTag = false;
      if(pt.y() < 15 * 10) // FIXME
      {
        for(int i = 0; i < m_BookmarkTags.size() && !onTag; i++)
        {
          if (m_BookmarkTags[i].first.contains(event->pos()))
            onTag = true;
        }
      }
      // if no mouse button monitor grab regions and change cursor icon
      if (onTag)
      {
        setCursor(QCursor(Qt::PointingHandCursor));
        m_CursorCaptured = BOOKMARK;
      }
      else if (isPointCloseTo(pt.x(), m_DemodFreqX, m_CursorCaptureDelta))
      {
        // in move demod box center frequency region
        if (CENTER != m_CursorCaptured)
          setCursor(QCursor(Qt::SizeHorCursor));
        m_CursorCaptured = CENTER;
        if (m_TooltipsEnabled)
          QToolTip::showText(
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
              event->globalPosition().toPoint(),
#else
              event->globalPos(),
#endif
              QString("Demod: %1 kHz")
              .arg(m_DemodCenterFreq/1.e3f, 0, 'f', 3),
              this);
      }
      else if (isPointCloseTo(pt.x(), m_DemodHiCutFreqX, m_CursorCaptureDelta))
      {
        // in move demod hicut region
        if (RIGHT != m_CursorCaptured)
          setCursor(QCursor(Qt::SizeFDiagCursor));
        m_CursorCaptured = RIGHT;
        if (m_TooltipsEnabled)
          QToolTip::showText(
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
              event->globalPosition().toPoint(),
#else
              event->globalPos(),
#endif
              QString("High cut: %1 Hz")
              .arg(m_DemodHiCutFreq),
              this);
      }
      else if (isPointCloseTo(pt.x(), m_DemodLowCutFreqX, m_CursorCaptureDelta))
      {
        // in move demod lowcut region
        if (LEFT != m_CursorCaptured)
          setCursor(QCursor(Qt::SizeBDiagCursor));
        m_CursorCaptured = LEFT;
        if (m_TooltipsEnabled)
          QToolTip::showText(
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
              event->globalPosition().toPoint(),
#else
              event->globalPos(),
#endif
              QString("Low cut: %1 Hz")
              .arg(m_DemodLowCutFreq),
              this);
      }
      else if (isPointCloseTo(pt.x(), m_YAxisWidth/2, m_YAxisWidth/2))
      {
        if (YAXIS != m_CursorCaptured)
          setCursor(QCursor(Qt::OpenHandCursor));
        m_CursorCaptured = YAXIS;
        if (m_TooltipsEnabled)
          QToolTip::hideText();
      }
      else if (isPointCloseTo(pt.y(), m_XAxisYCenter, m_CursorCaptureDelta+5))
      {
        if (XAXIS != m_CursorCaptured)
          setCursor(QCursor(Qt::OpenHandCursor));
        m_CursorCaptured = XAXIS;
        if (m_TooltipsEnabled)
          QToolTip::hideText();
      }
      else
      {	//if not near any grab boundaries
        if (NOCAP != m_CursorCaptured)
        {
          setCursor(QCursor(Qt::ArrowCursor));
          m_CursorCaptured = NOCAP;
        }
        if (m_TooltipsEnabled)
          QToolTip::showText(
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
              event->globalPosition().toPoint(),
#else
              event->globalPos(),
#endif
              QString("F: %1 kHz")
              .arg(freqFromX(pt.x())/1.e3f, 0, 'f', 3),
              this);
      }
      m_GrabPosition = 0;
    }
  }
  else
  {
    // not in Overlay region
    if (event->buttons() == Qt::NoButton)
    {
      if (NOCAP != m_CursorCaptured)
        setCursor(QCursor(Qt::ArrowCursor));

      m_CursorCaptured = NOCAP;
      m_GrabPosition = 0;
    }
    if (m_TooltipsEnabled)
    {
      QDateTime tt;
      tt.setMSecsSinceEpoch(msecFromY(pt.y()));

      QToolTip::showText(
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
          event->globalPosition().toPoint(),
#else
          event->globalPos(),
#endif
          QString("%1\n%2 kHz")
          .arg(tt.toString("yyyy.MM.dd hh:mm:ss.zzz"))
          .arg(freqFromX(pt.x())/1.e3f, 0, 'f', 3),
          this);
    }
  }
  // process mouse moves while in cursor capture modes
  if (YAXIS == m_CursorCaptured)
  {
    if (event->buttons() & Qt::LeftButton)
    {
      setCursor(QCursor(Qt::ClosedHandCursor));
      // move Y scale up/down
      float delta_px = m_Yzero - pt.y();
      float delta_db = delta_px * fabs(m_PandMindB - m_PandMaxdB) /
        (float)m_OverlayPixmap.height();
      m_PandMindB -= delta_db;
      m_PandMaxdB -= delta_db;
      if (out_of_range(m_PandMindB, m_PandMaxdB))
      {
        m_PandMindB += delta_db;
        m_PandMaxdB += delta_db;
      }
      else
      {
        emit pandapterRangeChanged(m_PandMindB, m_PandMaxdB);

        updateOverlay();

        m_PeakHoldValid = false;

        m_Yzero = pt.y();
      }
    }
  }
  else if (XAXIS == m_CursorCaptured)
  {

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (event->buttons() & (Qt::LeftButton | Qt::MiddleButton))
#else
      if (event->buttons() & (Qt::LeftButton | Qt::MidButton))
#endif // QT_VERSION
      {
        setCursor(QCursor(Qt::ClosedHandCursor));
        // pan viewable range or move center frequency
        int delta_px = m_Xzero - pt.x();
        qint64 delta_hz = delta_px * m_Span / m_OverlayPixmap.width();
        if (event->buttons() & m_freqDragBtn)
        {
          if (!m_Locked && !m_freqDragLocked) {
            qint64 centerFreq = boundCenterFreq(m_CenterFreq + delta_hz);
            delta_hz = centerFreq - m_CenterFreq;

            m_CenterFreq += delta_hz;
            m_DemodCenterFreq += delta_hz;

            ////////////// TODO: Edit this to fake spectrum scroll /////////
            ////////////// DONE: Something like this:
            ///
            m_tentativeCenterFreq += delta_hz;

            if (delta_hz != 0)
              emit newCenterFreq(m_CenterFreq);
          }
        } else {
          setFftCenterFreq(m_FftCenter + delta_hz);
        }

        updateOverlay();

        m_PeakHoldValid = false;

        m_Xzero = pt.x();
      }
  }
  else if (LEFT == m_CursorCaptured)
  {
    // moving in demod lowcut region
    if (event->buttons() & (Qt::LeftButton | Qt::RightButton))
    {
      // moving in demod lowcut region with left button held
      if (m_GrabPosition != 0)
      {
        m_DemodLowCutFreq = freqFromX(pt.x() - m_GrabPosition ) - m_DemodCenterFreq;
        m_DemodLowCutFreq = roundFreq(m_DemodLowCutFreq, m_FilterClickResolution);

        if (m_symetric && (event->buttons() & Qt::LeftButton))  // symetric adjustment
        {
          m_DemodHiCutFreq = -m_DemodLowCutFreq;
        }
        clampDemodParameters();

        emit newFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
        updateOverlay();
      }
      else
      {
        // save initial grab postion from m_DemodFreqX
        m_GrabPosition = pt.x()-m_DemodLowCutFreqX;
      }
    }
    else if (event->buttons() & ~Qt::NoButton)
    {
      setCursor(QCursor(Qt::ArrowCursor));
      m_CursorCaptured = NOCAP;
    }
  }
  else if (RIGHT == m_CursorCaptured)
  {
    // moving in demod highcut region
    if (event->buttons() & (Qt::LeftButton | Qt::RightButton))
    {
      // moving in demod highcut region with right button held
      if (m_GrabPosition != 0)
      {
        m_DemodHiCutFreq = freqFromX( pt.x()-m_GrabPosition ) - m_DemodCenterFreq;
        m_DemodHiCutFreq = roundFreq(m_DemodHiCutFreq, m_FilterClickResolution);

        if (m_symetric && (event->buttons() & Qt::LeftButton)) // symetric adjustment
        {
          m_DemodLowCutFreq = -m_DemodHiCutFreq;
        }
        clampDemodParameters();

        emit newFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
        updateOverlay();
      }
      else
      {
        // save initial grab postion from m_DemodFreqX
        m_GrabPosition = pt.x() - m_DemodHiCutFreqX;
      }
    }
    else if (event->buttons() & ~Qt::NoButton)
    {
      setCursor(QCursor(Qt::ArrowCursor));
      m_CursorCaptured = NOCAP;
    }
  }
  else if (CENTER == m_CursorCaptured)
  {
    // moving inbetween demod lowcut and highcut region
    if (event->buttons() & Qt::LeftButton)
    {   // moving inbetween demod lowcut and highcut region with left button held
      if (m_GrabPosition != 0)
      {
        if (!m_Locked) {
          m_DemodCenterFreq = roundFreq(freqFromX(pt.x() - m_GrabPosition),
              m_ClickResolution );
          emit newDemodFreq(m_DemodCenterFreq,
              m_DemodCenterFreq - m_CenterFreq);
          updateOverlay();
          m_PeakHoldValid = false;
        }
      }
      else
      {
        // save initial grab postion from m_DemodFreqX
        m_GrabPosition = pt.x() - m_DemodFreqX;
      }
    }
    else if (event->buttons() & ~Qt::NoButton)
    {
      setCursor(QCursor(Qt::ArrowCursor));
      m_CursorCaptured = NOCAP;
    }
  }
  else
  {
    // cursor not captured
    m_GrabPosition = 0;
  }
  if (!this->rect().contains(pt))
  {
    if (NOCAP != m_CursorCaptured)
      setCursor(QCursor(Qt::ArrowCursor));
    m_CursorCaptured = NOCAP;
  }
}

// Called by QT when screen needs to be redrawn
void AbstractWaterfall::paintEvent(QPaintEvent *ev)
{
  QOpenGLWidget::paintEvent(ev);
  QPainter painter(this);
  qint64  StartFreq = m_CenterFreq + m_FftCenter - m_Span / 2;
  qint64  EndFreq = StartFreq + m_Span;

  painter.setRenderHint(QPainter::Antialiasing);
  painter.drawPixmap(0, 0, m_2DPixmap);
  this->drawWaterfall(painter);

  // Draw named channel cutoffs
  if (m_channelsEnabled) {
    for (auto i = m_channelSet.find(StartFreq); i != m_channelSet.cend(); ++i) {
      auto p = i.value();
      int x_fCenter = xFromFreq(p->frequency);
      int x_fMin = xFromFreq(p->frequency + p->lowFreqCut);
      int x_fMax = xFromFreq(p->frequency + p->highFreqCut);

      if (EndFreq < p->frequency + p->lowFreqCut)
        break;

      WFHelpers::drawChannelCutoff(
          painter,
          m_SpectrumPlotHeight,
          x_fMin,
          x_fMax,
          x_fCenter,
          p->markerColor,
          p->cutOffColor,
          !p->bandLike);
    }
  }

  // Draw demod filter box
  if (m_FilterBoxEnabled) {
    this->drawFilterBox(painter, m_SpectrumPlotHeight);
    this->drawFilterCutoff(painter, m_SpectrumPlotHeight);
  }

  if (m_TimeStampsEnabled)
    paintTimeStamps(
        painter,
        QRect(2, m_SpectrumPlotHeight, this->width(), this->height()));
}

int AbstractWaterfall::getNearestPeak(QPoint pt)
{
  QMap<int, int>::const_iterator i = m_Peaks.lowerBound(pt.x() - PEAK_CLICK_MAX_H_DISTANCE);
  QMap<int, int>::const_iterator upperBound = m_Peaks.upperBound(pt.x() + PEAK_CLICK_MAX_H_DISTANCE);
  float   dist = 1.0e10;
  int     best = -1;

  for ( ; i != upperBound; i++)
  {
    int x = i.key();
    int y = i.value();

    if (abs(y - pt.y()) > PEAK_CLICK_MAX_V_DISTANCE)
      continue;

    float d = powf(y - pt.y(), 2) + powf(x - pt.x(), 2);
    if (d < dist)
    {
      dist = d;
      best = x;
    }
  }

  return best;
}

/** Set waterfall span in milliseconds */
void AbstractWaterfall::setWaterfallSpan(quint64 span_ms)
{
  int dpi_factor = isHdpiAware() ? screen()->devicePixelRatio() : 1;
  wf_span = span_ms;
  if (m_WaterfallHeight > 0)
    msec_per_wfline = wf_span / (m_WaterfallHeight * dpi_factor);
  clearWaterfall();
}

/** Get waterfall time resolution in milleconds / line. */
double AbstractWaterfall::getWfTimeRes(void)
{
  if (msec_per_wfline)
    return msec_per_wfline;
  else
    return 1000.0 / fft_rate; // Auto mode
}

void AbstractWaterfall::setFftRate(int rate_hz)
{
  fft_rate = rate_hz;
  clearWaterfall();
}

// Called when a mouse button is pressed
void AbstractWaterfall::mousePressEvent(QMouseEvent * event)
{
  QPoint pt = event->pos();

  if (NOCAP == m_CursorCaptured)
  {
    if (isPointCloseTo(pt.x(), m_DemodFreqX, m_CursorCaptureDelta))
    {
      // move demod box center frequency region
      m_CursorCaptured = CENTER;
      m_GrabPosition = pt.x() - m_DemodFreqX;
    }
    else if (isPointCloseTo(pt.x(), m_DemodLowCutFreqX, m_CursorCaptureDelta))
    {
      // filter low cut
      m_CursorCaptured = LEFT;
      m_GrabPosition = pt.x() - m_DemodLowCutFreqX;
    }
    else if (isPointCloseTo(pt.x(), m_DemodHiCutFreqX, m_CursorCaptureDelta))
    {
      // filter high cut
      m_CursorCaptured = RIGHT;
      m_GrabPosition = pt.x() - m_DemodHiCutFreqX;
    }
    else
    {
      if (event->buttons() == Qt::LeftButton)
      {
        if (!m_Locked) {
          int     best = -1;

          if (m_PeakDetection > 0)
            best = getNearestPeak(pt);
          if (best != -1)
            m_DemodCenterFreq = freqFromX(best);
          else
            m_DemodCenterFreq = roundFreq(freqFromX(pt.x()), m_ClickResolution);

          // if cursor not captured set demod frequency and start demod box capture
          emit newDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq - m_CenterFreq);

          // save initial grab postion from m_DemodFreqX
          // setCursor(QCursor(Qt::CrossCursor));
          m_CursorCaptured = CENTER;
          m_GrabPosition = 1;
          updateOverlay();
        }
      }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
      else if (event->buttons() == Qt::MiddleButton)
#else
      else if (event->buttons() == Qt::MidButton)
#endif // QT_VERSION
      {
        if (!m_Locked && !m_freqDragLocked) {
          // set center freq
          m_CenterFreq
            = boundCenterFreq(roundFreq(freqFromX(pt.x()), m_ClickResolution));

          m_DemodCenterFreq = m_CenterFreq;
          emit newCenterFreq(m_CenterFreq);
          emit newDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq - m_CenterFreq);
          updateOverlay();
        }
      }
      else if (event->buttons() == Qt::RightButton)
      {
        // reset frequency zoom
        resetHorizontalZoom();
        updateOverlay();
      }
    }
  }
  else
  {
    if (m_CursorCaptured == YAXIS)
      // get ready for moving Y axis
      m_Yzero = pt.y();
    else if (m_CursorCaptured == XAXIS)
    {
      m_Xzero = pt.x();
      if (event->buttons() == Qt::RightButton)
      {
        // reset frequency zoom
        resetHorizontalZoom();
        updateOverlay();
      }
    }
    else if (m_CursorCaptured == BOOKMARK)
    {
      if (!m_Locked) {
        for (int i = 0; i < m_BookmarkTags.size(); i++)
        {
          if (m_BookmarkTags[i].first.contains(event->pos()))
          {
            BookmarkInfo info = m_BookmarkTags[i].second;

            if (!info.modulation.isEmpty()) {
              emit newModulation(info.modulation);
            }

            m_DemodCenterFreq = info.frequency;
            emit newDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq - m_CenterFreq);

            if (info.bandwidth() != 0) {
              emit newFilterFreq(info.lowFreqCut, info.highFreqCut);
            }

            break;
          }
        }
      }
    }
  }
}

void AbstractWaterfall::mouseReleaseEvent(QMouseEvent * event)
{
  QPoint pt = event->pos();

  if (!m_OverlayPixmap.rect().contains(pt))
  {
    // not in Overlay region
    if (NOCAP != m_CursorCaptured)
      setCursor(QCursor(Qt::ArrowCursor));

    m_CursorCaptured = NOCAP;
    m_GrabPosition = 0;
  }
  else
  {
    if (YAXIS == m_CursorCaptured)
    {
      setCursor(QCursor(Qt::OpenHandCursor));
      m_Yzero = -1;
    }
    else if (XAXIS == m_CursorCaptured)
    {
      setCursor(QCursor(Qt::OpenHandCursor));
      m_Xzero = -1;
    }
  }
}


// Make a single zoom step on the X axis.
void AbstractWaterfall::zoomStepX(float step, int x)
{
  // calculate new range shown on FFT
  float new_range = qBound(10.0f,
      (float)(m_Span) * step,
      (float)(m_SampleFreq) * 10.0f);

  // Frequency where event occured is kept fixed under mouse
  float ratio = (float)x / (float)m_OverlayPixmap.width();
  float fixed_hz = freqFromX(x);
  float f_max = fixed_hz + (1.0 - ratio) * new_range;
  float f_min = f_max - new_range;

  qint64 fc = (qint64)(f_min + (f_max - f_min) / 2.0);

  setFftCenterFreq(fc - m_CenterFreq);
  setSpanFreq(new_range);

  float factor = (float)m_SampleFreq / (float)m_Span;
  emit newZoomLevel(factor);

  m_PeakHoldValid = false;
}

// Zoom on X axis (absolute level)
void AbstractWaterfall::zoomOnXAxis(float level)
{
  float current_level = (float)m_SampleFreq / (float)m_Span;

  zoomStepX(current_level / level, xFromFreq(m_DemodCenterFreq));
}

// Called when a mouse wheel is turned
void AbstractWaterfall::wheelEvent(QWheelEvent * event)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  QPointF pt = event->position();
#else
  QPointF pt = event->pos();
#endif // QT_VERSION

  // delta is in eigths of a degree, 15 degrees is one step
  double numSteps = event->angleDelta().y() / (8.0 * 15.0);

  if (m_CursorCaptured == YAXIS)
  {
    // Vertical zoom. Wheel down: zoom out, wheel up: zoom in
    // During zoom we try to keep the point (dB or kHz) under the cursor fixed
    qreal zoom_fac = pow(0.9, numSteps);
    qreal ratio = pt.y() / m_OverlayPixmap.height();
    qreal db_range = m_PandMaxdB - m_PandMindB;
    qreal y_range = m_OverlayPixmap.height();
    qreal db_per_pix = db_range / y_range;
    qreal fixed_db = m_PandMaxdB - pt.y() * db_per_pix;

    db_range = qBound(
        10.,
        db_range * zoom_fac,
        SCAST(qreal, FFT_MAX_DB - FFT_MIN_DB));
    m_PandMaxdB = fixed_db + ratio * db_range;
    if (m_PandMaxdB > FFT_MAX_DB)
      m_PandMaxdB = FFT_MAX_DB;

    m_PandMindB = m_PandMaxdB - db_range;
    if (m_PandMindB < FFT_MIN_DB)
      m_PandMindB = FFT_MIN_DB;

    m_PeakHoldValid = false;

    emit pandapterRangeChanged(m_PandMindB, m_PandMaxdB);
  }
  else if (m_CursorCaptured == XAXIS)
  {
    zoomStepX(pow(0.9, numSteps), pt.x());
  }
  else if (event->modifiers() & Qt::ControlModifier)
  {
    // filter width
    m_DemodLowCutFreq -= numSteps * m_ClickResolution;
    m_DemodHiCutFreq += numSteps * m_ClickResolution;
    clampDemodParameters();
    emit newFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
  }

  else if (event->modifiers() & Qt::ShiftModifier)
  {
    if (!m_Locked) {
      // filter shift
      m_DemodLowCutFreq += numSteps * m_ClickResolution;
      m_DemodHiCutFreq += numSteps * m_ClickResolution;
      clampDemodParameters();
      emit newFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
    }
  }
  else
  {
    if (!m_Locked) {
      // small steps will be lost by roundFreq, let them accumulate
      m_CumWheelDelta += event->angleDelta().y();
      if (abs(m_CumWheelDelta) < 8*15)
        return;
      numSteps = m_CumWheelDelta / (8.0 * 15.0);

      // inc/dec demod frequency
      m_DemodCenterFreq += (numSteps * m_ClickResolution);
      m_DemodCenterFreq = roundFreq(m_DemodCenterFreq, m_ClickResolution );
      emit newDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq-m_CenterFreq);
    }
  }

  updateOverlay();
  m_CumWheelDelta = 0;
}

// Called when screen size changes so must recalculate bitmaps
void AbstractWaterfall::resizeEvent(QResizeEvent* event)
{
  int dpi_factor = isHdpiAware() ? screen()->devicePixelRatio() : 1;

  // mandatory to call for QOpenGLWidget to resize framebuffer
  if (event != nullptr)
    QOpenGLWidget::resizeEvent(event);

  if (!size().isValid())
    return;

  if (m_Size != size())
  {
    // if changed, resize pixmaps to new screensize
    m_Size = size();
    m_SpectrumPlotHeight = m_Percent2DScreen * m_Size.height() / 100;
    m_WaterfallHeight = m_Size.height() - m_SpectrumPlotHeight;
    m_OverlayPixmap = QPixmap(m_Size.width(), m_SpectrumPlotHeight);
    m_OverlayPixmap.fill(Qt::black);
    m_2DPixmap = QPixmap(m_Size.width(), m_SpectrumPlotHeight);
    m_2DPixmap.fill(Qt::black);

    m_PeakHoldValid = false;

    if (wf_span > 0)
      msec_per_wfline = wf_span / (m_WaterfallHeight * dpi_factor);
  }

  updateOverlay();
}

void AbstractWaterfall::paintTimeStamps(
    QPainter &painter,
    QRect const &where)
{
  QFontMetrics metrics(m_Font);
  int y = where.y();
  int textWidth;
  int textHeight = metrics.height();
  int items = 0;
  int leftSpacing = 0;
  int dpi_factor = isHdpiAware() ? screen()->devicePixelRatio() : 1;

  auto it = m_TimeStamps.begin();

  painter.setFont(m_Font);

  y += m_TimeStampCounter / dpi_factor;

  if (m_TimeStampMaxHeight < where.height())
    m_TimeStampMaxHeight = where.height();

  painter.setPen(m_TimeStampColor);

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  leftSpacing = metrics.horizontalAdvance("00:00:00.000");
#else
  leftSpacing = metrics.width("00:00:00.000");
#endif // QT_VERSION_CHECK

  while (y < m_TimeStampMaxHeight + textHeight && it != m_TimeStamps.end()) {
    QString const &timeStampText =
      m_TimeStampsUTC ? it->utcTimeStampText : it->timeStampText;
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    textWidth = metrics.horizontalAdvance(timeStampText);
#else
    textWidth = metrics.width(it->timeStampText);
#endif // QT_VERSION_CHECK

    if (it->marker) {
      painter.drawText(
          where.x() + where.width() - textWidth - 2,
          y - 2,
          timeStampText);
      painter.drawLine(where.x() + leftSpacing, y, where.width() - 1, y);
    } else {
      painter.drawText(where.x(), y - 2, timeStampText);
      painter.drawLine(where.x(), y, textWidth + where.x(), y);
    }

    y += it->counter / dpi_factor;
    ++it;
    ++items;
  }

  // TimeStamps from here could not be painted due to geometry restrictions.
  // we silently discard them.

  while (items < m_TimeStamps.size())
    m_TimeStamps.removeLast();
}

  void
AbstractWaterfall::drawChannelBoxAndCutoff(
    QPainter &painter,
    int h,
    qint64 fMin,
    qint64 fMax,
    qint64 fCenter,
    QColor boxColor,
    QColor markerColor,
    QColor cutOffColor,
    QString text,
    QColor textColor)
{

  int x_fCenter = xFromFreq(fCenter);
  int x_fMin = xFromFreq(fMin);
  int x_fMax = xFromFreq(fMax);

  WFHelpers::drawChannelBox(
      painter,
      h,
      x_fMin,
      x_fMax,
      x_fCenter,
      boxColor,
      markerColor,
      text,
      textColor);

  WFHelpers::drawChannelCutoff(
      painter,
      h,
      x_fMin,
      x_fMax,
      x_fCenter,
      markerColor,
      cutOffColor);
}

  void
AbstractWaterfall::drawFilterBox(QPainter &painter, int h)
{
  m_DemodFreqX = xFromFreq(m_DemodCenterFreq);
  m_DemodLowCutFreqX = xFromFreq(m_DemodCenterFreq + m_DemodLowCutFreq);
  m_DemodHiCutFreqX = xFromFreq(m_DemodCenterFreq + m_DemodHiCutFreq);

  WFHelpers::drawChannelBox(
      painter,
      h,
      m_DemodLowCutFreqX,
      m_DemodHiCutFreqX,
      m_DemodFreqX,
      m_FilterBoxColor,
      QColor(PLOTTER_FILTER_LINE_COLOR),
      "",
      QColor());
}

  void
AbstractWaterfall::drawFilterCutoff(QPainter &painter, int h)
{
  m_DemodFreqX = xFromFreq(m_DemodCenterFreq);
  m_DemodLowCutFreqX = xFromFreq(m_DemodCenterFreq + m_DemodLowCutFreq);
  m_DemodHiCutFreqX = xFromFreq(m_DemodCenterFreq + m_DemodHiCutFreq);

  WFHelpers::drawChannelCutoff(
      painter,
      h,
      m_DemodLowCutFreqX,
      m_DemodHiCutFreqX,
      m_DemodFreqX,
      QColor(PLOTTER_FILTER_LINE_COLOR),
      m_TimeStampColor);
}

/**
 * Set new FFT data.
 * @param fftData Pointer to the new FFT data (same data for pandapter and waterfall).
 * @param size The FFT size.
 *
 * When FFT data is set using this method, the same data will be used for both the
 * pandapter and the waterfall.
 */
void AbstractWaterfall::setNewFftData(
    const float *fftData,
    int size,
    QDateTime const &t,
    bool looped)
{
  this->setNewFftData(fftData, fftData, size, t, looped);
}

/**
 * Set new FFT data.
 * @param fftData Pointer to the new FFT data used on the pandapter.
 * @param wfData Pointer to the FFT data used in the waterfall.
 * @param size The FFT size.
 *
 * This method can be used to set different FFT data set for the pandapter and the
 * waterfall.
 */
void AbstractWaterfall::setNewFftData(
    const float *fftData,
    const float *wfData,
    int size,
    QDateTime const &t,
    bool looped)
{
  /** FIXME **/
  if (!m_Running)
    m_Running = true;

  quint64 tnow_ms = SCAST(quint64, t.toMSecsSinceEpoch());

  if (looped) {
    TimeStamp ts;

    ts.counter = m_TimeStampCounter;
    ts.timeStampText =
      m_lastFft.toLocalTime().toString("hh:mm:ss.zzz")
      + " - "
      + t.toLocalTime().toString("hh:mm:ss.zzz");
    ts.utcTimeStampText =
      m_lastFft.toUTC().toString("hh:mm:ss.zzzZ")
      + " - "
      + t.toUTC().toString("hh:mm:ss.zzzZ");
    ts.marker = true;

    m_TimeStamps.push_front(ts);
    m_TimeStampCounter = 0;
  }

  m_fftData = fftData;
  m_fftDataSize = size;
  m_lastFft = t;

  if (m_tentativeCenterFreq != 0) {
    m_tentativeCenterFreq = 0;
    m_DrawOverlay = true;
  }

  if (m_TimeStampCounter >= m_TimeStampSpacing) {
    TimeStamp ts;

    ts.counter = m_TimeStampCounter;
    ts.timeStampText = t.toLocalTime().toString("hh:mm:ss.zzz");
    ts.utcTimeStampText = t.toUTC().toString("hh:mm:ss.zzzZ");

    m_TimeStamps.push_front(ts);
    m_TimeStampCounter = 0;
  }

  if (wfData != nullptr && size > 0) {
    if (msec_per_wfline > 0) {
      this->accumulateFftData(wfData, size);

      if (tnow_ms < tlast_wf_ms || tnow_ms - tlast_wf_ms >= msec_per_wfline) {
        int line_count = (tnow_ms - tlast_wf_ms) / msec_per_wfline;
        if (line_count >= 1 && line_count <= 20) {
          tlast_wf_ms += msec_per_wfline * line_count;
        } else {
          line_count = 1;
          tlast_wf_ms = tnow_ms;
        }
        this->averageFftData();
        this->addNewWfLine(m_accum.data(), size, line_count);
        this->resetFftAccumulator();
        m_TimeStampCounter += line_count;
      }
    } else {
      tlast_wf_ms = tnow_ms;
      this->addNewWfLine(wfData, size, 1);
      ++m_TimeStampCounter;
    }
  }

  draw();
}

void AbstractWaterfall::getScreenIntegerFFTData(qint32 plotHeight, qint32 plotWidth,
    float maxdB, float mindB,
    qint64 startFreq, qint64 stopFreq,
    const float *inBuf, qint32 *outBuf,
    int *xmin, int *xmax)
{
  qint32 i;
  qint32 y;
  qint32 x;
  qint32 ymax = 10000;
  qint32 xprev = -1;
  qint32 minbin, maxbin;
  qint32 m_BinMin, m_BinMax;
  qint32 m_FFTSize = m_fftDataSize;
  const float *m_pFFTAveBuf = inBuf;

  mindB -= m_gain;
  maxdB -= m_gain;

  float  dBGainFactor = ((float)plotHeight) / fabs(maxdB - mindB);
  qint32* m_pTranslateTbl = new qint32[qMax(m_FFTSize, plotWidth)];

  /** FIXME: qint64 -> qint32 **/
  m_BinMin = (qint32)((float)startFreq * (float)m_FFTSize / m_SampleFreq);
  m_BinMin += (m_FFTSize/2);
  m_BinMax = (qint32)((float)stopFreq * (float)m_FFTSize / m_SampleFreq);
  m_BinMax += (m_FFTSize/2);

  minbin = qBound(0, m_BinMin, m_FFTSize - 1);
  maxbin = qBound(0, m_BinMax, m_FFTSize - 1);

  bool largeFft = (maxbin - minbin) > plotWidth; // true if more fft point than plot points

  if (largeFft)
  {
    // more FFT points than plot points
    for (i = minbin; i < maxbin; i++)
      m_pTranslateTbl[i] = ((qint64)(i-m_BinMin)*plotWidth) / (m_BinMax - m_BinMin);
    *xmin = m_pTranslateTbl[minbin];
    *xmax = m_pTranslateTbl[maxbin - 1];
  }
  else
  {
    // more plot points than FFT points
    for (i = 0; i < plotWidth; i++)
      m_pTranslateTbl[i] = m_BinMin + (i*(m_BinMax - m_BinMin)) / plotWidth;
    *xmin = 0;
    *xmax = plotWidth;
  }

  if (largeFft)
  {
    // more FFT points than plot points
    for (i = minbin; i < maxbin; i++ )
    {
      y = (qint32)(dBGainFactor*(maxdB-m_pFFTAveBuf[i]));

      if (y > plotHeight)
        y = plotHeight;
      else if (y < 0)
        y = 0;

      x = m_pTranslateTbl[i];	//get fft bin to plot x coordinate transform

      if (x == xprev)   // still mappped to same fft bin coordinate
      {
        if (y < ymax) // store only the max value
        {
          outBuf[x] = y;
          ymax = y;
        }

      }
      else
      {
        outBuf[x] = y;
        xprev = x;
        ymax = y;
      }
    }
  }
  else
  {
    // more plot points than FFT points
    for (x = 0; x < plotWidth; x++ )
    {
      i = m_pTranslateTbl[x]; // get plot to fft bin coordinate transform
      if(i < 0 || i >= m_FFTSize)
        y = plotHeight;
      else
        y = (qint32)(dBGainFactor*(maxdB-m_pFFTAveBuf[i]));

      if (y > plotHeight)
        y = plotHeight;
      else if (y < 0)
        y = 0;

      outBuf[x] = y;
    }
  }

  delete [] m_pTranslateTbl;
}

void AbstractWaterfall::setFftRange(float min, float max)
{
  setWaterfallRange(min, max);
  setPandapterRange(min, max);
}

void AbstractWaterfall::setPandapterRange(float min, float max)
{
  if (out_of_range(min, max))
    return;

  m_PandMindB = min;
  m_PandMaxdB = max;
  updateOverlay();
  m_PeakHoldValid = false;
}

void AbstractWaterfall::setWaterfallRange(float min, float max)
{
  if (out_of_range(min, max))
    return;

  m_WfMindB = min;
  m_WfMaxdB = max;
  // no overlay change is necessary
}

void AbstractWaterfall::setInfoText(QString const &text)
{
  m_infoText = text;
  updateOverlay();
}

void AbstractWaterfall::setInfoTextColor(QColor const &color)
{
  m_infoTextColor = color;
  updateOverlay();
}

  static QString
formatFreqUnits(int units)
{
  switch (units) {
    case 1:
      return QString("");

    case 1000:
      return QString("K");

    case 1000000:
      return QString("M");

    case 1000000000:
      return QString("G");
  }

  return QString("");
}


// Create frequency division strings based on start frequency, span frequency,
// and frequency units.
// Places in QString array m_HDivText
// Keeps all strings the same fractional length
void AbstractWaterfall::makeFrequencyStrs()
{
  qint64  StartFreq = m_StartFreqAdj;
  float   freq;
  int     i,j, centerDiv;
  bool    deltaRep;

  if ((1 == m_FreqUnits) || (m_FreqDigits == 0))
  {
    // if units is Hz then just output integer freq
    for (int i = 0; i <= m_HorDivs; i++)
    {
      freq = (float)StartFreq/(float)m_FreqUnits;
      m_HDivText[i].setNum((int)freq);
      StartFreq += m_FreqPerDiv;
    }
    return;
  }
  // here if is fractional frequency values
  // so create max sized text based on frequency units
  for (int i = 0; i <= m_HorDivs; i++)
  {
    freq = (float)StartFreq / (float)m_FreqUnits;
    m_HDivText[i].setNum(freq,'f', m_FreqDigits);
    StartFreq += m_FreqPerDiv;
  }

  // now find the division text with the longest non-zero digit
  // to the right of the decimal point.
  int max = 0;
  for (i = 0; i <= m_HorDivs; i++)
  {
    int dp = m_HDivText[i].indexOf('.');
    int l = m_HDivText[i].length()-1;
    for (j = l; j > dp; j--)
    {
      if (m_HDivText[i][j] != '0')
        break;
    }
    if ((j - dp) > max)
      max = j - dp;
  }

  // Decide whether we should use the delta representation here. We identify
  // this situation if log10(m_FreqPerDiv) - log10(m_FreqUnits) <= -m_FreqDigits.
  // This is, if the division in axis units requires more decimals than
  // the current representation, settle for delta representation

  deltaRep = log10(m_FreqPerDiv) - log10(m_FreqUnits) <= -m_FreqDigits;

  // truncate all strings to maximum fractional length
  StartFreq  = m_StartFreqAdj;
  centerDiv  = m_HorDivs / 2;

  if (deltaRep) {
    for (i = 0; i <= m_HorDivs; i++) {
      if (i == centerDiv) {
        m_HDivText[i] = SuWidgetsHelpers::formatQuantityFromDelta(
            StartFreq,
            m_FreqPerDiv,
            "Hz",
            false);
      } else {
        m_HDivText[i] = SuWidgetsHelpers::formatQuantityFromDelta(
            (i - centerDiv) * m_FreqPerDiv,
            m_FreqPerDiv,
            "Hz",
            true);
      }

      StartFreq += m_FreqPerDiv;
    }
  } else {
    for (i = 0; i <= m_HorDivs; i++) {
      freq = (float)StartFreq/(float)m_FreqUnits;
      m_HDivText[i].setNum(freq,'f', max);
      m_HDivText[i] += formatFreqUnits(m_FreqUnits);
      StartFreq += m_FreqPerDiv;
    }
  }
}
// Convert from screen coordinate to frequency
int AbstractWaterfall::xFromFreq(qint64 freq)
{
  int w = m_OverlayPixmap.width();
  qint64 StartFreq = m_CenterFreq + m_FftCenter - m_Span/2;
  int x = w * ((qreal)freq - StartFreq)/(qreal)m_Span;
  if (x < 0)
    return 0;
  if (x > w)
    return w;
  return x;
}

// Convert from frequency to screen coordinate
qint64 AbstractWaterfall::freqFromX(int x)
{
  int w = m_OverlayPixmap.width();
  qint64 StartFreq = m_CenterFreq + m_FftCenter - m_Span / 2;
  qint64 f = (qint64)(StartFreq + (qreal)m_Span * (qreal)x / (qreal)w);
  return f;
}

/** Calculate time offset of a given line on the waterfall */
quint64 AbstractWaterfall::msecFromY(int y)
{
  // ensure we are in the waterfall region
  if (y < m_OverlayPixmap.height())
    return 0;

  int dy = y - m_OverlayPixmap.height();

  if (msec_per_wfline > 0)
    return tlast_wf_ms - dy * msec_per_wfline;
  else
    return tlast_wf_ms - dy * 1000 / fft_rate;
}

// Round frequency to click resolution value
qint64 AbstractWaterfall::roundFreq(qint64 freq, int resolution)
{
  qint64 delta = resolution;
  qint64 delta_2 = delta / 2;
  if (freq >= 0)
    return (freq - (freq + delta_2) % delta + delta_2);
  else
    return (freq - (freq + delta_2) % delta - delta_2);
}

// Clamp demod freqeuency limits of m_DemodCenterFreq
void AbstractWaterfall::clampDemodParameters()
{
  if(m_DemodLowCutFreq < m_FLowCmin)
    m_DemodLowCutFreq = m_FLowCmin;
  if(m_DemodLowCutFreq > m_FLowCmax)
    m_DemodLowCutFreq = m_FLowCmax;

  if(m_DemodHiCutFreq < m_FHiCmin)
    m_DemodHiCutFreq = m_FHiCmin;
  if(m_DemodHiCutFreq > m_FHiCmax)
    m_DemodHiCutFreq = m_FHiCmax;
}

void AbstractWaterfall::setDemodRanges(qint64 FLowCmin, qint64 FLowCmax,
    qint64 FHiCmin, qint64 FHiCmax,
    bool symetric)
{
  m_FLowCmin=FLowCmin;
  m_FLowCmax=FLowCmax;
  m_FHiCmin=FHiCmin;
  m_FHiCmax=FHiCmax;
  m_symetric=symetric;
  clampDemodParameters();
  updateOverlay();
}

void AbstractWaterfall::setCenterFreq(qint64 f)
{
  f = boundCenterFreq(f);

  if(m_CenterFreq == f)
    return;

  m_tentativeCenterFreq += f - m_CenterFreq;
  m_CenterFreq = f;

  updateOverlay();

  m_PeakHoldValid = false;
}

void AbstractWaterfall::setFrequencyLimits(qint64 min, qint64 max)
{
  this->m_lowerFreqLimit = min;
  this->m_upperFreqLimit = max;

  if (this->m_enforceFreqLimits)
    this->setCenterFreq(this->m_CenterFreq);
}

void AbstractWaterfall::setFrequencyLimitsEnabled(bool enabled)
{
  this->m_enforceFreqLimits = enabled;

  if (this->m_enforceFreqLimits)
    this->setCenterFreq(this->m_CenterFreq);
}

NamedChannelSetIterator AbstractWaterfall::addChannel(
    QString name,
    qint64 frequency,
    qint32 fMin,
    qint32 fMax,
    QColor boxColor,
    QColor markerColor,
    QColor cutOffColor)
{
  auto it = m_channelSet.addChannel(
      name,
      frequency,
      fMin,
      fMax,
      boxColor,
      markerColor,
      cutOffColor);

  refreshChannel(it);

  return it;
}

void AbstractWaterfall::removeChannel(NamedChannelSetIterator it)
{
  m_channelSet.remove(it);

  updateOverlay();
}

void AbstractWaterfall::refreshChannel(NamedChannelSetIterator &it)
{

  if (m_channelSet.isOutOfPlace(it))
    it = m_channelSet.relocate(it);

  updateOverlay();
}

NamedChannelSetIterator AbstractWaterfall::findChannel(qint64 freq)
{
  return m_channelSet.find(freq);
}

NamedChannelSetIterator AbstractWaterfall::channelCBegin() const
{
  return m_channelSet.cbegin();
}

NamedChannelSetIterator AbstractWaterfall::channelCEnd() const
{
  return m_channelSet.cend();
}


// Ensure overlay is updated by either scheduling or forcing a redraw
void AbstractWaterfall::updateOverlay()
{
  m_DrawOverlay = true;

  // draw immediately if there won't be new FFT data coming soon to triger redraw
  if (!m_Running || this->slow())
    draw();
}

/** Reset horizontal zoom to 100% and centered around 0. */
void AbstractWaterfall::resetHorizontalZoom(void)
{
  setFftCenterFreq(0);
  setSpanFreq(static_cast<qint64>(m_SampleFreq));
  emit newZoomLevel(1);
}

/** Center FFT plot around 0 (corresponds to center freq). */
void AbstractWaterfall::moveToCenterFreq(void)
{
  setFftCenterFreq(0);
  updateOverlay();
  m_PeakHoldValid = false;
}

/** Center FFT plot around the demodulator frequency. */
void AbstractWaterfall::moveToDemodFreq(void)
{
  setFftCenterFreq(m_DemodCenterFreq-m_CenterFreq);
  updateOverlay();

  m_PeakHoldValid = false;
}

/** Set FFT plot color. */
void AbstractWaterfall::setFftPlotColor(const QColor color)
{
  m_FftColor = color;
  m_FftFillCol = color;
  m_FftFillCol.setAlpha(0x1A);
  m_PeakHoldColor = color;
  m_PeakHoldColor.setAlpha(60);
  updateOverlay();
}

/** Set filter box color */
void AbstractWaterfall::setFilterBoxColor(const QColor color)
{
  m_FilterBoxColor = color;
  updateOverlay();
}

/** Set timestamp color */
void AbstractWaterfall::setTimeStampColor(const QColor color)
{
  m_TimeStampColor = color;
  updateOverlay();
}

/** Set FFT bg color. */
void AbstractWaterfall::setFftBgColor(const QColor color)
{
  m_FftBgColor = color;
  updateOverlay();
}

/** Set FFT axes color. */
void AbstractWaterfall::setFftAxesColor(const QColor color)
{
  m_FftCenterAxisColor = color;
  m_FftAxesColor = color;
}

/** Set FFT text color. */
void AbstractWaterfall::setFftTextColor(const QColor color)
{
  m_FftTextColor = color;
  updateOverlay();
}

/** Enable/disable filling the area below the FFT plot. */
void AbstractWaterfall::setFftFill(bool enabled)
{
  m_FftFill = enabled;
}

/** Set peak hold on or off. */
void AbstractWaterfall::setPeakHold(bool enabled)
{
  m_PeakHoldActive = enabled;
  m_PeakHoldValid = false;
}

/**
 * Set peak detection on or off.
 * @param enabled The new state of peak detection.
 * @param c Minimum distance of peaks from mean, in multiples of standard deviation.
 */
void AbstractWaterfall::setPeakDetection(bool enabled, float c)
{
  if(!enabled || c <= 0)
    m_PeakDetection = -1;
  else
    m_PeakDetection = c;
}

void AbstractWaterfall::calcDivSize (qint64 low, qint64 high, int divswanted, qint64 &adjlow, qint64 &step, int& divs)
{
#ifdef PLOTTER_DEBUG
  qDebug() << "low: " << low;
  qDebug() << "high: " << high;
  qDebug() << "divswanted: " << divswanted;
#endif

  if (divswanted == 0)
    return;

  static const qint64 stepTable[] = { 1, 2, 5 };
  static const int stepTableSize = sizeof (stepTable) / sizeof (stepTable[0]);
  qint64 multiplier = 1;
  step = 1;
  qint64 divsLong = high - low;
  int index = 0;
  adjlow = (low / step) * step;

  while (divsLong > divswanted)
  {
    step = stepTable[index] * multiplier;
    divsLong = (high - low) / step;
    adjlow = (low / step) * step;
    index = index + 1;
    if (index == stepTableSize)
    {
      index = 0;
      multiplier = multiplier * 10;
    }
  }
  if (adjlow < low)
    adjlow += step;


  divs = static_cast<int>(divsLong);

#ifdef PLOTTER_DEBUG
  qDebug() << "adjlow: "  << adjlow;
  qDebug() << "step: " << step;
  qDebug() << "divs: " << divs;
#endif
}

void AbstractWaterfall::pushFAT(const FrequencyAllocationTable *fat)
{
  this->m_FATs[fat->getName()] = fat;

  if (this->m_ShowFATs)
    this->updateOverlay();
}

bool AbstractWaterfall::removeFAT(std::string const &name)
{
  auto p = this->m_FATs.find(name);

  if (p == this->m_FATs.end())
    return false;

  this->m_FATs.erase(p);

  if (this->m_ShowFATs)
    this->updateOverlay();

  return true;
}

void AbstractWaterfall::setFATsVisible(bool visible)
{
  this->m_ShowFATs = visible;
  this->updateOverlay();
}

int AbstractWaterfall::drawFATs(
    DrawingContext &ctx,
    qint64 StartFreq,
    qint64 EndFreq)
{
  int count = 0;
  int w = ctx.width;
  int h = ctx.height;
  QString label;
  QRect rect;

  for (auto fat : m_FATs) {
    if (fat.second != nullptr) {
      FrequencyBandIterator p = fat.second->find(StartFreq);

      while (p != fat.second->cbegin() && p->second.max > StartFreq)
        --p;

      for (; p != fat.second->cend() && p->second.min < EndFreq; ++p) {
        int x0 = xFromFreq(p->second.min);
        int x1 = xFromFreq(p->second.max);
        bool leftborder = true;
        bool rightborder = true;
        int tw, boxw;


        if (x0 < m_YAxisWidth) {
          leftborder = false;
          x0 = m_YAxisWidth;
        }

        if (x1 >= w) {
          rightborder = false;
          x1 = w - 1;
        }

        if (x1 < m_YAxisWidth)
          continue;

        boxw = x1 - x0;

        ctx.painter->setBrush(QBrush(p->second.color));
        ctx.painter->setPen(p->second.color);

        ctx.painter->drawRect(
            x0,
            count * ctx.metrics->height(),
            x1 - x0 + 1,
            ctx.metrics->height());

        if (leftborder)
          ctx.painter->drawLine(
              x0,
              count * ctx.metrics->height(),
              x0,
              h);

        if (rightborder)
          ctx.painter->drawLine(
              x1,
              count * ctx.metrics->height(),
              x1,
              h);

        label = ctx.metrics->elidedText(
            QString::fromStdString(p->second.primary),
            Qt::ElideRight,
            boxw);

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        tw = ctx.metrics->horizontalAdvance(label);
#else
        tw = ctx.metrics->width(label);
#endif // QT_VERSION_CHECK

        if (tw < boxw) {
          ctx.painter->setPen(m_FftTextColor);
          rect.setRect(
              x0 + (x1 - x0) / 2 - tw / 2,
              count * ctx.metrics->height(),
              tw,
              ctx.metrics->height());
          ctx.painter->drawText(rect, Qt::AlignHCenter | Qt::AlignVCenter, label);
        }
      }

      ++count;
    }
  }

  return count * ctx.metrics->height();
}

void AbstractWaterfall::drawBookmarks(
    DrawingContext &ctx,
    qint64 StartFreq,
    qint64 EndFreq,
    int xAxisTop)
{
  m_BookmarkTags.clear();
  int fontHeight = ctx.metrics->ascent() + 1;
  int slant = 5;
  int levelHeight = fontHeight + 5;
  int x;
  const int nLevels = 10;

  QList<BookmarkInfo> bookmarks =
    m_BookmarkSource->getBookmarksInRange(
        StartFreq,
        EndFreq);
  int tagEnd[nLevels] = {0};

  for (int i = 0; i < bookmarks.size(); i++) {
    x = xFromFreq(bookmarks[i].frequency);
#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64)
    int nameWidth = ctx.metrics->width(bookmarks[i].name);
#else
    int nameWidth = ctx.metrics->boundingRect(bookmarks[i].name).width();
#endif

    int level = 0;
    int yMin = static_cast<int>(m_FATs.size()) * ctx.metrics->height();
    while (level < nLevels && tagEnd[level] > x)
      level++;

    if (level == nLevels)
      level = 0;

    tagEnd[level] = x + nameWidth + slant - 1;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    BookmarkInfo bookmark = bookmarks[i];
    m_BookmarkTags.append(
        qMakePair<QRect, BookmarkInfo>(
          QRect(x, yMin + level * levelHeight, nameWidth + slant, fontHeight),
          std::move(bookmark))); // Be more Cobol every day
#else
    m_BookmarkTags.append(
        qMakePair<QRect, BookmarkInfo>(
          QRect(x, yMin + level * levelHeight, nameWidth + slant, fontHeight),
          bookmarks[i]));
#endif // QT_VERSION

    QColor color = QColor(bookmarks[i].color);
    color.setAlpha(0x60);
    // Vertical line
    ctx.painter->setPen(
        QPen(color, 1, Qt::DashLine));
    ctx.painter->drawLine(
        x,
        yMin + level * levelHeight + fontHeight + slant,
        x,
        xAxisTop);

    // Horizontal line
    ctx.painter->setPen(
        QPen(color, 1, Qt::SolidLine));
    ctx.painter->drawLine(
        x + slant, yMin + level * levelHeight + fontHeight,
        x + nameWidth + slant - 1,
        yMin + level * levelHeight + fontHeight);
    // Diagonal line
    ctx.painter->drawLine(
        x + 1,
        yMin + level * levelHeight + fontHeight + slant - 1,
        x + slant - 1,
        yMin + level * levelHeight + fontHeight + 1);

    color.setAlpha(0xFF);
    ctx.painter->setPen(QPen(color, 2, Qt::SolidLine));
    ctx.painter->drawText(
        x + slant,
        yMin + level * levelHeight,
        nameWidth,
        fontHeight,
        Qt::AlignVCenter | Qt::AlignHCenter,
        bookmarks[i].name);
  }
}

void AbstractWaterfall::drawAxes(DrawingContext &ctx, qint64 StartFreq, qint64 EndFreq)
{
  int w = ctx.width;
  int h = ctx.height;

  int     x,y;
  float   pixperdiv;
  float   adjoffset;
  float   unitStepSize;
  float   minUnitAdj;
  QRect   rect;

  // solid background
  ctx.painter->setBrush(Qt::SolidPattern);
  ctx.painter->fillRect(0, 0, w, h, m_FftBgColor);

#define HOR_MARGIN 5
#define VER_MARGIN 5

  // X and Y axis areas
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  int tw = ctx.metrics->horizontalAdvance("XXXX");
#else
  int tw = ctx.metrics->width("XXXX");
#endif // QT_VERSION_CHECK

  m_YAxisWidth = tw + 2 * HOR_MARGIN;
  m_XAxisYCenter = h - ctx.metrics->height() / 2;
  int xAxisHeight = ctx.metrics->height() + 2 * VER_MARGIN;
  int xAxisTop = h - xAxisHeight;
  int fLabelTop = xAxisTop + VER_MARGIN;

  if (m_CenterLineEnabled) {
    x = xFromFreq(m_CenterFreq - m_tentativeCenterFreq);
    if (x > 0 && x < w) {
      ctx.painter->setPen(m_FftCenterAxisColor);
      ctx.painter->drawLine(x, 0, x, xAxisTop);
    }
  }

  // Frequency grid
  QString label;
  label.setNum(float(EndFreq / m_FreqUnits), 'f', m_FreqDigits);

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  tw = ctx.metrics->horizontalAdvance(label) + ctx.metrics->horizontalAdvance("O");
#else
  tw = ctx.metrics->width(label) + ctx.metrics->width("O");
#endif // QT_VERSION_CHECK

  calcDivSize(
      StartFreq,
      EndFreq,
      qMin(w / tw, HORZ_DIVS_MAX),
      m_StartFreqAdj,
      m_FreqPerDiv,
      m_HorDivs);

  pixperdiv = (float)w * (float) m_FreqPerDiv / (float) m_Span;
  adjoffset = pixperdiv * float (m_StartFreqAdj - StartFreq) / (float) m_FreqPerDiv;

  ctx.painter->setPen(QPen(m_FftAxesColor, 1, Qt::DotLine));
  for (int i = 0; i <= m_HorDivs; i++) {
    x = (int)((float)i * pixperdiv + adjoffset);
    if (x > m_YAxisWidth)
      ctx.painter->drawLine(x, 0, x, xAxisTop);
  }

  // draw frequency values (x axis)
  makeFrequencyStrs();
  ctx.painter->setPen(m_FftTextColor);
  for (int i = 0; i <= m_HorDivs; i++) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    int tw = ctx.metrics->horizontalAdvance(m_HDivText[i]);
#else
    int tw = ctx.metrics->width(m_HDivText[i]);
#endif // QT_VERSION_CHECK
    x = (int)((float)i*pixperdiv + adjoffset);
    if (x > m_YAxisWidth) {
      rect.setRect(x - tw/2, fLabelTop, tw, ctx.metrics->height());
      ctx.painter->drawText(rect, Qt::AlignHCenter|Qt::AlignBottom, m_HDivText[i]);
    }
  }


  // Level grid
  qint64 minUnitAdj64 = 0;
  qint64 unitDivSize = 0;
  qint64 unitSign    = m_dBPerUnit < 0 ? -1 : 1;
  qint64 pandMinUnit = unitSign * static_cast<qint64>(toDisplayUnits(m_PandMindB));
  qint64 pandMaxUnit = unitSign * static_cast<qint64>(toDisplayUnits(m_PandMaxdB));

  calcDivSize(pandMinUnit, pandMaxUnit,
      qMax(h/m_VdivDelta, VERT_DIVS_MIN), minUnitAdj64, unitDivSize,
      m_VerDivs);

  unitStepSize = (float) unitDivSize;
  minUnitAdj = minUnitAdj64;

  pixperdiv = (float) h * (float) unitStepSize / (pandMaxUnit - pandMinUnit);
  adjoffset = (float) h * (minUnitAdj - pandMinUnit) / (pandMaxUnit - pandMinUnit);

#ifdef PLOTTER_DEBUG
  qDebug() << "minDb =" << m_PandMindB << "maxDb =" << m_PandMaxdB
    << "mindbadj =" << mindbadj << "dbstepsize =" << dbstepsize
    << "pixperdiv =" << pixperdiv << "adjoffset =" << adjoffset;
#endif

  ctx.painter->setPen(QPen(m_FftAxesColor, 1, Qt::DotLine));
  for (int i = 0; i <= m_VerDivs; i++) {
    y = h - (int)((float) i * pixperdiv + adjoffset);
    if (y < h - xAxisHeight)
      ctx.painter->drawLine(m_YAxisWidth, y, w, y);
  }

  // draw amplitude values (y axis)
  int unit;
  int unitWidth;

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  m_YAxisWidth = ctx.metrics->horizontalAdvance("-160 ");
  unitWidth    = ctx.metrics->horizontalAdvance(m_unitName);
#else
  m_YAxisWidth = ctx.metrics->width("-160 ");
  unitWidth    = ctx.metrics->width(m_unitName);
#endif // QT_VERSION_CHECK

  if (unitWidth > m_YAxisWidth)
    m_YAxisWidth = unitWidth;

  ctx.painter->setPen(m_FftTextColor);
  int th = ctx.metrics->height();
  for (int i = 0; i < m_VerDivs; i++) {
    y = h - (int)((float) i * pixperdiv + adjoffset);

    if (y < h -xAxisHeight) {
      unit = minUnitAdj + unitStepSize * i;
      rect.setRect(HOR_MARGIN, y - th / 2, m_YAxisWidth, th);
      ctx.painter->drawText(
          rect,
          Qt::AlignRight|Qt::AlignVCenter,
          QString::number(unitSign * unit));
    }
  }

  // Draw unit name on top left corner
  rect.setRect(HOR_MARGIN, 0, unitWidth, th);
  ctx.painter->drawText(rect, Qt::AlignRight|Qt::AlignVCenter, m_unitName);

#ifdef GL_WATERFALL_BOOKMARKS_SUPPORT
  if (m_BookmarksEnabled && m_BookmarkSource != nullptr)
    this->drawBookmarks(ctx, StartFreq, EndFreq, xAxisTop);
#endif // GL_WATERFALL_BOOKMARKS_SUPPORT
}

// Called to draw an overlay bitmap containing grid and text that
// does not need to be recreated every fft data update.
void AbstractWaterfall::drawOverlay()
{
  if (m_OverlayPixmap.isNull())
    return;

  int             bandY = 0;
  QFontMetrics    metrics(m_Font);
  QPainter        painter(&m_OverlayPixmap);
  DrawingContext  ctx;

  qint64  StartFreq = m_CenterFreq + m_FftCenter - m_Span / 2;
  qint64  EndFreq = StartFreq + m_Span;

  ctx.painter = &painter;
  ctx.metrics = &metrics;
  ctx.width   = m_OverlayPixmap.width();
  ctx.height  = m_OverlayPixmap.height();

  painter.setFont(m_Font);

  // Draw axes
  this->drawAxes(ctx, StartFreq, EndFreq);

  // Draw frequency allocation tables
  if (m_ShowFATs)
    bandY = this->drawFATs(ctx, StartFreq, EndFreq);

  // Draw named channel (boxes)

  if (m_channelsEnabled) {
    for (auto i = m_channelSet.find(StartFreq - m_Span); i != m_channelSet.cend(); ++i) {
      auto p = i.value();
      int x_fCenter = xFromFreq(p->frequency);
      int x_fMin = xFromFreq(p->frequency + p->lowFreqCut);
      int x_fMax = xFromFreq(p->frequency + p->highFreqCut);

      if (p->frequency + p->highFreqCut < StartFreq)
        continue;

      if (EndFreq < p->frequency + p->lowFreqCut)
        break;

      if (p->bandLike) {
        WFHelpers::drawChannelBox(
            painter,
            ctx.height,
            x_fMin,
            x_fMax,
            x_fCenter,
            p->boxColor,
            p->markerColor,
            p->name,
            p->markerColor,
            ctx.metrics->height() / 2,
            bandY + p->nestLevel * ctx.metrics->height());
      } else {
        WFHelpers::drawChannelBox(
            painter,
            ctx.height,
            x_fMin,
            x_fMax,
            x_fCenter,
            p->boxColor,
            p->markerColor,
            p->name,
            QColor(),
            -1,
            bandY + p->nestLevel * ctx.metrics->height());
      }
    }
  }

  // Draw info text (if enabled)
  if (!m_infoText.isEmpty()) {
    int flags = Qt::AlignRight |  Qt::AlignTop | Qt::TextWordWrap;
    QRectF pixRect = m_OverlayPixmap.rect();

    pixRect.setWidth(pixRect.width() - 10);

    QRectF rect = painter.boundingRect(
        pixRect,
        flags,
        m_infoText);

    rect.setX(pixRect.width() - rect.width());
    rect.setY(0);

    painter.setPen(QPen(m_infoTextColor, 2, Qt::SolidLine));
    painter.drawText(rect, flags, m_infoText);
  }

  painter.end();
}

void AbstractWaterfall::accumulateFftData(const float *fftData, int size)
{
  int i;

  if (m_accum.size() != static_cast<size_t>(size)) {
    m_accum.resize(static_cast<size_t>(size));
    this->resetFftAccumulator();
  }

  if (m_samplesInAccum == 0) {
    std::memcpy(m_accum.data(), fftData, size * sizeof(float));
  } else {
    for (i = 0; i < size; ++i)
      m_accum[i] += fftData[i];
  }

  m_samplesInAccum++;
}

void AbstractWaterfall::averageFftData()
{
  if (m_samplesInAccum == 0)
    return;

  float f = 1.0f / m_samplesInAccum;
  for (size_t i = 0; i < m_accum.size(); i++)
    m_accum[i] *= f;

  // avoids repeated calls causing issues
  m_samplesInAccum = 1;
}

void AbstractWaterfall::resetFftAccumulator()
{
  std::fill(m_accum.begin(), m_accum.end(), 0);
  m_samplesInAccum = 0;
}

void AbstractWaterfall::drawSpectrum(QPainter &painter)
{
  int     i, n;
  int     xmin, xmax;
  qint64  limit = ((qint64)m_SampleFreq + m_Span) / 2 - 1;
  QPoint  LineBuf[MAX_SCREENSIZE];
  int w = painter.device()->width();
  int h = painter.device()->height();

  // workaround for "fixed" line drawing since Qt 5
  // see http://stackoverflow.com/questions/16990326
#if QT_VERSION >= 0x050000
  painter.translate(0.5, 0.5);
#endif

  // Do we have valid FFT data?
  if (m_fftDataSize < 1)
    return;

  // get new scaled fft data
  getScreenIntegerFFTData(
      h,
      qMin(w, MAX_SCREENSIZE),
      m_PandMaxdB,
      m_PandMindB,
      qBound(
        -limit,
        m_tentativeCenterFreq + m_FftCenter,
        limit) - (qint64)m_Span/2,
      qBound(
        -limit,
        m_tentativeCenterFreq + m_FftCenter,
        limit) + (qint64)m_Span/2,
      m_fftData,
      m_fftbuf,
      &xmin,
      &xmax);

  // draw the pandapter
  painter.setPen(m_FftColor);
  n = xmax - xmin;
  for (i = 0; i < n; i++) {
    LineBuf[i].setX(i + xmin);
    LineBuf[i].setY(m_fftbuf[i + xmin]);
  }

  if (m_FftFill) {
    painter.setBrush(QBrush(m_FftFillCol, Qt::SolidPattern));
    if (n < MAX_SCREENSIZE-2) {
      LineBuf[n].setX(xmax-1);
      LineBuf[n].setY(h);
      LineBuf[n+1].setX(xmin);
      LineBuf[n+1].setY(h);
      painter.drawPolygon(LineBuf, n+2);
    } else {
      LineBuf[MAX_SCREENSIZE-2].setX(xmax-1);
      LineBuf[MAX_SCREENSIZE-2].setY(h);
      LineBuf[MAX_SCREENSIZE-1].setX(xmin);
      LineBuf[MAX_SCREENSIZE-1].setY(h);
      painter.drawPolygon(LineBuf, n);
    }
  } else {
    painter.drawPolyline(LineBuf, n);
  }

  // Peak detection
  if (m_PeakDetection > 0) {
    m_Peaks.clear();

    float   mean = 0;
    float   sum_of_sq = 0;
    for (i = 0; i < n; i++) {
      mean += m_fftbuf[i + xmin];
      sum_of_sq += m_fftbuf[i + xmin] * m_fftbuf[i + xmin];
    }
    mean /= n;
    float stdev= sqrt(sum_of_sq / n - mean * mean );

    int lastPeak = -1;
    for (i = 0; i < n; i++) {
      //m_PeakDetection times the std over the mean or better than current peak
      float d = (lastPeak == -1) ? (mean - m_PeakDetection * stdev) :
        m_fftbuf[lastPeak + xmin];

      if (m_fftbuf[i + xmin] < d)
        lastPeak=i;

      if (lastPeak != -1 &&
          (i - lastPeak > PEAK_H_TOLERANCE || i == n-1))
      {
        m_Peaks.insert(lastPeak + xmin, m_fftbuf[lastPeak + xmin]);
        painter.drawEllipse(lastPeak + xmin - 5,
            m_fftbuf[lastPeak + xmin] - 5, 10, 10);
        lastPeak = -1;
      }
    }
  }

  // Peak hold
  if (m_PeakHoldActive) {
    for (i = 0; i < n; i++) {
      if (!m_PeakHoldValid || m_fftbuf[i] < m_fftPeakHoldBuf[i])
        m_fftPeakHoldBuf[i] = m_fftbuf[i];

      LineBuf[i].setX(i + xmin);
      LineBuf[i].setY(m_fftPeakHoldBuf[i + xmin]);
    }
    painter.setPen(m_PeakHoldColor);
    painter.drawPolyline(LineBuf, n);

    m_PeakHoldValid = true;
  }
}

// Called to update spectrum data for displaying on the screen
void AbstractWaterfall::draw()
{
  int     w;
  int     h;

  if (m_DrawOverlay) {
    drawOverlay();
    m_DrawOverlay = false;
  }

  // -----8<------------------------------------------------------------------
  // In the loving memory of a waterfall drawing code that use to hog my CPU
  // for years now. It's sad it's gone, I'm glad is not here anymore.
  // -----8<------------------------------------------------------------------

  // get/draw the 2D spectrum
  w = m_2DPixmap.width();
  h = m_2DPixmap.height();

  if (w != 0 && h != 0) {
    // first copy into 2Dbitmap the overlay bitmap.
    m_2DPixmap = m_OverlayPixmap.copy(0, 0, w, h);

    // draw the pandapter spectrum over the overlay (really underlay)
    QPainter painter(&m_2DPixmap);
    drawSpectrum(painter);
  }

  // trigger a new paintEvent
  update();
}
