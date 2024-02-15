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
#ifndef ABSTRACT_WATERFALL_H
#define ABSTRACT_WATERFALL_H

#include <QtGui>
#include <QFont>
#include <QFrame>
#include <QList>
#include <vector>
#include <QMap>
#include <QOpenGLWidget>

#define WATERFALL_BOOKMARKS_SUPPORT

#include "WFHelpers.h"

struct DrawingContext {
  QPainter     *painter;
  QFontMetrics *metrics;
  int width;
  int height;
};

class AbstractWaterfall : public QOpenGLWidget
{
  Q_OBJECT

  public:
    explicit AbstractWaterfall(QWidget *parent = 0);
    ~AbstractWaterfall();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

    void draw(); // call to draw new fft data onto screen plot
    void setLocked(bool locked) { m_Locked = locked; }
    void setFreqDragLocked(bool locked) { m_freqDragLocked = locked; }
    void setRunningState(bool running) { m_Running = running; }
    void setClickResolution(int clickres) { m_ClickResolution = clickres; }
    void setExpectedRate(int rate) { m_expectedRate = rate; }
    void setFilterClickResolution(int clickres) { m_FilterClickResolution = clickres; }
    void setFilterBoxEnabled(bool enabled) { m_FilterBoxEnabled = enabled; }
    void setCenterLineEnabled(bool enabled) { m_CenterLineEnabled = enabled; }
    void setTooltipsEnabled(bool enabled) { m_TooltipsEnabled = enabled; }
    void setBookmarksEnabled(bool enabled) { m_BookmarksEnabled = enabled; }
    void setTimeStampsEnabled(bool enabled) { m_TimeStampsEnabled = enabled; }
    void setTimeStampsUTC(bool utc) { m_TimeStampsUTC = utc; drawOverlay(); }

    virtual bool isGLWaterfall() { return false; }

    void setChannelsEnabled(bool enabled) { m_channelsEnabled = enabled; updateOverlay(); }

    void setUseLBMdrag(bool enabled)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
      m_freqDragBtn = (enabled) ? Qt::LeftButton :Qt::MiddleButton;
#else
      m_freqDragBtn = (enabled) ? Qt::LeftButton :Qt::MidButton;
#endif // QT_VERSION
    }

#ifdef WATERFALL_BOOKMARKS_SUPPORT
    void setBookmarkSource(BookmarkSource *src) { m_BookmarkSource = src; }
#endif // WATERFALL_BOOKMARKS_SUPPORT

    bool removeFAT(std::string const &);
    void pushFAT(const FrequencyAllocationTable *);
    void setFATsVisible(bool);

    void setNewFftData(
        const float *fftData,
        int size,
        QDateTime const &t = QDateTime::currentDateTime(),
        bool looped = false);

    void setNewFftData(
        const float *fftData,
        const float *wfData,
        int size,
        QDateTime const &t = QDateTime::currentDateTime(),
        bool looped = false);

    virtual void setPalette(const QColor *table) = 0;

    virtual void setMaxBlending(bool val)
    {
      (void)val; // suppress unused variable warning
    }

    void setCenterFreq(qint64 f);
    void setFreqUnits(qint32 unit) { m_FreqUnits = unit; }

    void setDemodCenterFreq(qint64 f) { m_DemodCenterFreq = f; }

    bool slow(void) const
    {
      if (m_fftDataSize == 0)
        return true;

      if (m_expectedRate != 0 && m_expectedRate < MINIMUM_REFRESH_RATE)
        return true;

      return m_SampleFreq / m_fftDataSize < MINIMUM_REFRESH_RATE;
    }

    /*! \brief Move the filter to freq_hz from center. */
    void setFilterOffset(qint64 freq_hz)
    {
      m_DemodCenterFreq = m_CenterFreq + freq_hz;
      drawOverlay();
    }
    qint64 getFilterOffset(void)
    {
      return m_DemodCenterFreq - m_CenterFreq;
    }

    qint64 getFilterBw()
    {
      return m_DemodHiCutFreq - m_DemodLowCutFreq;
    }

    void setHiLowCutFrequencies(qint64 LowCut, qint64 HiCut)
    {
      m_DemodLowCutFreq = LowCut;
      m_DemodHiCutFreq = HiCut;
      drawOverlay();
    }

    void setdBPerUnit(float dBPerUnit)
    {
      m_dBPerUnit = dBPerUnit;
      drawOverlay();
    }

    void setZeroPoint(float zeroPoint)
    {
      m_ZeroPoint = zeroPoint;
      drawOverlay();
    }

    void getHiLowCutFrequencies(qint64 *LowCut, qint64 *HiCut)
    {
      *LowCut = m_DemodLowCutFreq;
      *HiCut = m_DemodHiCutFreq;
    }

    void setDemodRanges(
        qint64 FLowCmin,
        qint64 FLowCmax,
        qint64 FHiCmin,
        qint64 FHiCmax,
        bool symetric);

    qint64 getCenterFreq(void) const
    {
      return m_CenterFreq;
    }

    float toDisplayUnits(float dB)
    {
      return dB / this->m_dBPerUnit - this->m_ZeroPoint;
    }

    void setGain(float gain)
    {
      m_gain = gain;
    }

    void setUnitName(QString const &name)
    {
      m_unitName = name;
    }

    QString getUnitName(void) const
    {
      return m_unitName;
    }

    /* Shown bandwidth around SetCenterFreq() */
    void setSpanFreq(qint64 s)
    {
      if (s > 0) {
        m_Span = s;
        setFftCenterFreq(m_FftCenter);
      }
      drawOverlay();
    }

    quint64 getSpanFreq(void) const
    {
      return static_cast<quint64>(this->m_Span);
    }

    qint64 getFftCenterFreq(void) const
    {
      return this->m_FftCenter;
    }

    void setHdivDelta(int delta) { m_HdivDelta = delta; }
    void setVdivDelta(int delta) { m_VdivDelta = delta; }

    void setFreqDigits(int digits) { m_FreqDigits = digits>=0 ? digits : 0; }

    /* Determines full bandwidth. */
    void setSampleRate(float rate)
    {
      if (rate > 0.0)
      {
        m_SampleFreq = rate;
        drawOverlay();
      }
    }

    float getSampleRate(void)
    {
      return m_SampleFreq;
    }

    void setFftCenterFreq(qint64 f) {
      qint64 limit = ((qint64)m_SampleFreq + m_Span) / 2 - 1;
      m_FftCenter = qBound(-limit, f, limit);
    }

    int     getNearestPeak(QPoint pt);
    void    setWaterfallSpan(quint64 span_ms);
    double  getWfTimeRes(void);
    void    setFftRate(int rate_hz);
    void    setFrequencyLimits(qint64 min, qint64 max);
    void    setFrequencyLimitsEnabled(bool);
    virtual void clearWaterfall() = 0;
    virtual bool saveWaterfall(const QString & filename) const = 0;

    NamedChannelSetIterator addChannel(
        QString name,
        qint64 frequency,
        qint32 fMin,
        qint32 fMax,
        QColor boxColor,
        QColor markerColor,
        QColor cutOffColor);

    void removeChannel(NamedChannelSetIterator);
    void refreshChannel(NamedChannelSetIterator &);
    NamedChannelSetIterator findChannel(qint64 freq);

    NamedChannelSetIterator channelCBegin() const;
    NamedChannelSetIterator channelCEnd() const;

  signals:
    void newCenterFreq(qint64 f);
    void newDemodFreq(qint64 freq, qint64 delta); /* delta is the offset from the center */
    void newLowCutFreq(int f);
    void newHighCutFreq(int f);
    void newModulation(QString modulation);
    void newFilterFreq(int low, int high);  /* substitute for NewLow / NewHigh */
    void pandapterRangeChanged(float min, float max);
    void newZoomLevel(float level);

  public slots:
    // zoom functions
    void resetHorizontalZoom(void);
    void moveToCenterFreq(void);
    void moveToDemodFreq(void);
    void zoomOnXAxis(float level);

    // other FFT slots
    void setFftPlotColor(const QColor color);
    void setFftBgColor(const QColor color);
    void setFftAxesColor(const QColor color);
    void setFftTextColor(const QColor color);
    void setFilterBoxColor(const QColor color);
    void setTimeStampColor(const QColor color);

    void setFftFill(bool enabled);
    void setPeakHold(bool enabled);
    void setFftRange(float min, float max);
    void setPandapterRange(float min, float max);
    virtual void setWaterfallRange(float min, float max);
    void setPeakDetection(bool enabled, float c);
    void updateOverlay();

    void setInfoText(QString const &);
    void setInfoTextColor(QColor const &);

    void setPercent2DScreen(int percent)
    {
      m_Percent2DScreen = percent;
      m_Size = QSize(0,0);
      resizeEvent(NULL);
    }

  protected:
    //re-implemented widget event handlers
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent* event);
    void mouseMoveEvent(QMouseEvent * event);
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void wheelEvent( QWheelEvent * event );

  protected:
    inline qint64 boundCenterFreq(qint64 f) const
    {
      if (this->m_enforceFreqLimits)
        return  qBound(this->m_lowerFreqLimit, f, this->m_upperFreqLimit);

      return f;
    }

    void drawChannelBoxAndCutoff(
        QPainter &painter,
        int h,
        qint64 fMin,
        qint64 fMax,
        qint64 fCenter,
        QColor boxColor,
        QColor markerColor,
        QColor cutOffColor,
        QString text = "",
        QColor textColor = QColor());
    void drawFilterBox(QPainter &painter, int height);
    void drawFilterCutoff(QPainter &painter, int height);

    enum eCapturetype {
      NOCAP,
      LEFT,
      CENTER,
      RIGHT,
      YAXIS,
      XAXIS,
      BOOKMARK
    };

    void        paintTimeStamps(QPainter &, QRect const &);
    void        drawOverlay();
    void        makeFrequencyStrs();
    int         xFromFreq(qint64 freq);
    qint64      freqFromX(int x);
    void        zoomStepX(float factor, int x);
    qint64      roundFreq(qint64 freq, int resolution);
    quint64     msecFromY(int y);
    void        clampDemodParameters();
    bool        isPointCloseTo(int x, int xr, int delta)
    {
      return ((x > (xr - delta)) && (x < (xr + delta)));
    }
    void getScreenIntegerFFTData(qint32 plotHeight, qint32 plotWidth,
        float maxdB, float mindB,
        qint64 startFreq, qint64 stopFreq,
        const float *inBuf, qint32 *outBuf,
        qint32 *maxbin, qint32 *minbin);
    void calcDivSize (qint64 low, qint64 high, int divswanted, qint64 &adjlow, qint64 &step, int& divs);

    int  drawFATs(DrawingContext &, qint64, qint64);
    void drawBookmarks(DrawingContext &, qint64, qint64, int xAxisTop);
    void drawAxes(DrawingContext &, qint64, qint64);
    void drawSpectrum(QPainter &);
    virtual void drawWaterfall(QPainter &) {}

    virtual void addNewWfLine(const float *wfData, int size, int repeats) = 0;

    void accumulateFftData(const float *fftData, int size);
    void averageFftData();
    void resetFftAccumulator();

    // FFT line averaging accumulator
    std::vector<float>  m_accum;
    int                 m_samplesInAccum;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    Qt::MouseButton m_freqDragBtn = Qt::MiddleButton;
#else
    Qt::MouseButton m_freqDragBtn = Qt::MidButton;
#endif // QT_VERSION

    bool        m_PeakHoldActive;
    bool        m_PeakHoldValid;
    qint32      m_fftbuf[MAX_SCREENSIZE];
    qint32      m_fftPeakHoldBuf[MAX_SCREENSIZE];
    const float *m_fftData = nullptr;   /*! pointer to incoming FFT data */
    int         m_fftDataSize = 0;

    int         m_XAxisYCenter;
    int         m_YAxisWidth;

    eCapturetype    m_CursorCaptured;
    QPixmap     m_2DPixmap;
    QPixmap     m_OverlayPixmap;
    QSize       m_Size;
    QString     m_Str;
    QString     m_HDivText[HORZ_DIVS_MAX+1];
    bool        m_Running;
    bool        m_DrawOverlay;
    qint64      m_CenterFreq;       // The HW frequency
    qint64      m_FftCenter;        // Center freq in the -span ... +span range
    qint64      m_DemodCenterFreq;
    qint64      m_StartFreqAdj;
    qint64      m_FreqPerDiv;
    bool        m_CenterLineEnabled;  /*!< Distinguish center line. */
    bool        m_FilterBoxEnabled;   /*!< Draw filter box. */
    bool        m_TooltipsEnabled;    /*!< Tooltips enabled */
    bool        m_BookmarksEnabled;   /*!< Show/hide bookmarks on spectrum */
    bool        m_Locked; /* Prevent manual adjust of center frequency */
    bool        m_freqDragLocked;
    qint64      m_DemodHiCutFreq;
    qint64      m_DemodLowCutFreq;
    int         m_DemodFreqX;		    // screen coordinate x position
    int         m_DemodHiCutFreqX;	// screen coordinate x position
    int         m_DemodLowCutFreqX;	// screen coordinate x position
    int         m_CursorCaptureDelta;
    int         m_GrabPosition;
    int         m_Percent2DScreen;
    int         m_SpectrumPlotHeight;
    int         m_WaterfallHeight;

    qint64      m_FLowCmin;
    qint64      m_FLowCmax;
    qint64      m_FHiCmin;
    qint64      m_FHiCmax;
    bool        m_symetric;

    int         m_HorDivs;   /*!< Current number of horizontal divisions. Calculated from width. */
    int         m_VerDivs;   /*!< Current number of vertical divisions. Calculated from height. */

    float       m_PandMindB;
    float       m_PandMaxdB;
    float       m_WfMindB;
    float       m_WfMaxdB;

    float       m_gain = 0.;
    float       m_ZeroPoint = 0;
    float       m_dBPerUnit = 1.;
    QString     m_unitName = "dBFS";

#ifdef WATERFALL_BOOKMARKS_SUPPORT
    BookmarkSource *m_BookmarkSource = nullptr;
#endif // WATERFALL_BOOKMARKS_SUPPORT
    qint64      m_Span;
    float       m_SampleFreq;    /*!< Sample rate. */
    qint32      m_FreqUnits;
    qint32      m_CumWheelDelta;
    int         m_ClickResolution;
    int         m_FilterClickResolution;

    int         m_Xzero;
    int         m_Yzero;  /*!< Used to measure mouse drag direction. */
    int         m_FreqDigits;  /*!< Number of decimal digits in frequency strings. */

    QFont       m_Font;         /*!< Font used for plotter (system font) */
    int         m_HdivDelta; /*!< Minimum distance in pixels between two horizontal grid lines (vertical division). */
    int         m_VdivDelta; /*!< Minimum distance in pixels between two vertical grid lines (horizontal division). */

    quint64     m_LastSampleRate;

    QColor      m_FftColor, m_FftFillCol, m_PeakHoldColor;
    QColor      m_FftBgColor, m_FftCenterAxisColor, m_FftAxesColor;
    QColor      m_FftTextColor;
    QColor      m_FilterBoxColor;
    QColor      m_TimeStampColor;
    bool        m_FftFill;

    qint64      m_tentativeCenterFreq = 0;
    float       m_PeakDetection;
    QMap<int,int>   m_Peaks;

#ifdef WATERFALL_BOOKMARKS_SUPPORT
    QList< QPair<QRect, BookmarkInfo> >     m_BookmarkTags;
#endif

    QDateTime   m_lastFft;
    QList<TimeStamp> m_TimeStamps;
    bool        m_TimeStampsEnabled = true;
    bool        m_TimeStampsUTC = true;
    int         m_TimeStampSpacing = 64;
    int         m_TimeStampCounter = 64;
    int         m_TimeStampMaxHeight = 0;

    // Frequency navigation limits
    bool        m_enforceFreqLimits = false;
    qint64      m_lowerFreqLimit    = 0;
    qint64      m_upperFreqLimit    = 300000000;

    // Waterfall averaging
    double      tlast_wf_ms;        // last time waterfall has been updated
    double      msec_per_wfline;    // milliseconds between waterfall updates
    double      wf_span;            // waterfall span in milliseconds (0 = auto)
    int         fft_rate;           // expected FFT rate (needed when WF span is auto)
    int         m_expectedRate;

    // Frequency allocations
    bool m_ShowFATs = false;
    std::map<std::string, const FrequencyAllocationTable *> m_FATs;

    // Named channels
    bool            m_channelsEnabled = true;
    NamedChannelSet m_channelSet;

    // Infotext
    QString m_infoText;
    QColor  m_infoTextColor;
};

#endif // ABSTRACT_WATERFALL_H
