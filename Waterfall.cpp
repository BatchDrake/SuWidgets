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

#include "Waterfall.h"
#include "SuWidgetsHelpers.h"

///////////////////////////// Waterfall ////////////////////////////////////////
Waterfall::Waterfall(QWidget *parent) : AbstractWaterfall(parent)
{
  m_WaterfallImage = QImage();

  // default waterfall color scheme
  for (int i = 0; i < 256; i++)
  {
    // level 0: black background
    if (i < 20)
      m_ColorTbl[i].setRgb(0, 0, 0);
    // level 1: black -> blue
    else if ((i >= 20) && (i < 70))
      m_ColorTbl[i].setRgb(0, 0, 140*(i-20)/50);
    // level 2: blue -> light-blue / greenish
    else if ((i >= 70) && (i < 100))
      m_ColorTbl[i].setRgb(60*(i-70)/30, 125*(i-70)/30, 115*(i-70)/30 + 140);
    // level 3: light blue -> yellow
    else if ((i >= 100) && (i < 150))
      m_ColorTbl[i].setRgb(195*(i-100)/50 + 60, 130*(i-100)/50 + 125, 255-(255*(i-100)/50));
    // level 4: yellow -> red
    else if ((i >= 150) && (i < 250))
      m_ColorTbl[i].setRgb(255, 255-255*(i-150)/100, 0);
    // level 5: red -> white
    else if (i >= 250)
      m_ColorTbl[i].setRgb(255, 255*(i-250)/5, 255*(i-250)/5);
  }

  for (int i = 0; i < 256; i++)
    m_UintColorTbl[i] = qRgb(
        m_ColorTbl[i].red(),
        m_ColorTbl[i].green(),
        m_ColorTbl[i].blue());
}

Waterfall::~Waterfall()
{
}

void Waterfall::setPalette(const QColor *table)
{
  unsigned int i;

  for (i = 0; i < 256; ++i) {
    this->m_ColorTbl[i] = table[i];
    this->m_UintColorTbl[i] = qRgb(
        table[i].red(),
        table[i].green(),
        table[i].blue());
  }

  this->update();
}

void Waterfall::clearWaterfall()
{
  m_WaterfallImage.fill(Qt::black);
}

/**
 * @brief Save waterfall to a graphics file
 * @param filename
 * @return TRUE if the save successful, FALSE if an erorr occurred.
 *
 * We assume that frequency strings are up to date
 */
bool Waterfall::saveWaterfall(const QString & filename) const
{
  QBrush          axis_brush(QColor(0x00, 0x00, 0x00, 0x70), Qt::SolidPattern);
  QPixmap         pixmap = QPixmap::fromImage(m_WaterfallImage);
  QPainter        painter(&pixmap);
  QRect           rect;
  QDateTime       tt;
  QFont           font("sans-serif");
  QFontMetrics    font_metrics(font);
  float           pixperdiv;
  int             x, y, w, h;
  int             hxa, wya = 85;
  int             i;

  w = pixmap.width();
  h = pixmap.height();
  hxa = font_metrics.height() + 5;    // height of X axis
  y = h - hxa;
  pixperdiv = (float) w / (float) m_HorDivs;

  painter.setBrush(axis_brush);
  painter.setPen(QColor(0x0, 0x0, 0x0, 0x70));
  painter.drawRect(0, y, w, hxa);
  painter.drawRect(0, 0, wya, h - hxa - 1);
  painter.setFont(font);
  painter.setPen(QColor(0xFF, 0xFF, 0xFF, 0xFF));

  // skip last frequency entry
  for (i = 2; i < m_HorDivs - 1; i++)
  {
    // frequency tick marks
    x = (int)((float)i * pixperdiv);
    painter.drawLine(x, y, x, y + 5);

    // frequency strings
    x = (int)((float)i * pixperdiv - pixperdiv / 2.0);
    rect.setRect(x, y, (int)pixperdiv, hxa);
    painter.drawText(rect, Qt::AlignHCenter|Qt::AlignBottom, m_HDivText[i]);
  }
  rect.setRect(w - pixperdiv - 10, y, pixperdiv, hxa);
  painter.drawText(rect, Qt::AlignRight|Qt::AlignBottom, tr("MHz"));

  quint64 msec;
  int tdivs = h / 70 + 1;
  pixperdiv = (float) h / (float) tdivs;
  tt.setTimeSpec(Qt::OffsetFromUTC);
  for (i = 1; i < tdivs; i++)
  {
    y = (int)((float)i * pixperdiv);
    if (msec_per_wfline > 0)
      msec =  tlast_wf_ms - y * msec_per_wfline;
    else
      msec =  tlast_wf_ms - y * 1000 / fft_rate;

    tt.setMSecsSinceEpoch(msec);
    rect.setRect(0, y - font_metrics.height(), wya - 5, font_metrics.height());
    painter.drawText(rect, Qt::AlignRight|Qt::AlignVCenter, tt.toString("yyyy.MM.dd"));
    painter.drawLine(wya - 5, y, wya, y);
    rect.setRect(0, y, wya - 5, font_metrics.height());
    painter.drawText(rect, Qt::AlignRight|Qt::AlignVCenter, tt.toString("hh:mm:ss"));
  }

  return pixmap.save(filename, 0, -1);
}

// Called when screen size changes so must recalculate bitmaps
void Waterfall::resizeEvent(QResizeEvent* event)
{
  AbstractWaterfall::resizeEvent(event);

  if (!size().isValid())
    return;

  if (m_WaterfallImage.isNull())
  {
    m_WaterfallImage = QImage(
        m_Size.width(),
        m_WaterfallHeight,
        QImage::Format::Format_RGB32);
    m_WaterfallImage.fill(Qt::black);
  }
  else if (m_WaterfallImage.width() != m_Size.width() ||
           m_WaterfallImage.height() != m_WaterfallHeight)
  {
    m_WaterfallImage = m_WaterfallImage.scaled(
        m_Size.width(),
        m_WaterfallHeight,
        Qt::IgnoreAspectRatio,
        Qt::SmoothTransformation);
  }
}

void Waterfall::addNewWfLine(const float* wfData, int size, int repeats)
{
  int w = m_WaterfallImage.width();
  int h = m_WaterfallImage.height();
  int xmin, xmax;
  qint64 limit = ((qint64)m_SampleFreq + m_Span) / 2 - 1;

  if (w == 0 || h == 0 || size == 0)
    return;

  // get scaled FFT data
  int n = qMin(w, MAX_SCREENSIZE);
  getScreenIntegerFFTData(255, n, m_WfMaxdB, m_WfMindB,
      qBound(
        -limit,
        m_tentativeCenterFreq + m_FftCenter,
        limit) - (qint64)m_Span/2,
      qBound(
        -limit,
        m_tentativeCenterFreq + m_FftCenter,
        limit) + (qint64)m_Span/2,
      wfData, m_fftbuf,
      &xmin, &xmax);

  // move current data down required amount (must do before attaching a QPainter object)
  memmove(
      m_WaterfallImage.scanLine(repeats),
      m_WaterfallImage.scanLine(0),
      static_cast<size_t>(w) * (static_cast<size_t>(h) - repeats)
      * sizeof(uint32_t));

  uint32_t *scanLineData =
    reinterpret_cast<uint32_t *>(m_WaterfallImage.scanLine(0));

  memset(
      scanLineData,
      0,
      static_cast<unsigned>(xmin) * sizeof(uint32_t));

  memset(
      scanLineData + xmax,
      0,
      static_cast<unsigned>(w - xmax) * sizeof(uint32_t));

  for (int i = xmin; i < xmax; i++)
    scanLineData[i] = m_UintColorTbl[255 - m_fftbuf[i]];

  // copy as needed onto extra lines
  for (int j = 1; j < repeats; j++) {
    uint32_t *subseqScanLineData =
      reinterpret_cast<uint32_t *>(m_WaterfallImage.scanLine(j));
    memcpy(subseqScanLineData, scanLineData,
        static_cast<size_t>(w) * sizeof(uint32_t));
  }
}

void Waterfall::drawWaterfall(QPainter &painter)
{
  painter.drawImage(0, m_SpectrumPlotHeight, m_WaterfallImage);
}
