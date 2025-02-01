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
#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <QFrame>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QList>
#include <QMap>

#include <sigutils/types.h>
#include "ThrottleableWidget.h"
#include "WaveView.h"

#define WAVEFORM_DEFAULT_BACKGROUND_COLOR QColor(0x1d, 0x1d, 0x1f)
#define WAVEFORM_DEFAULT_FOREGROUND_COLOR QColor(0xff, 0xff, 0x00)
#define WAVEFORM_DEFAULT_AXES_COLOR       QColor(0x34, 0x34, 0x34)
#define WAVEFORM_DEFAULT_TEXT_COLOR       QColor(0xff, 0xff, 0xff)
#define WAVEFORM_DEFAULT_SELECTION_COLOR  QColor(0x08, 0x08, 0x08)
#define WAVEFORM_DEFAULT_ENVELOPE_COLOR   QColor(0x3f, 0x3f, 0x00)
#define WAVEFORM_DEFAULT_SUBSEL_COLOR     QColor(0xff, 0x08, 0x08)
#define WAVEFORM_MAX_ITERS                20
#define WAVEFORM_DELTA_LIMIT              9000
#define WAVEFORM_POINT_RADIUS             5
#define WAVEFORM_POINT_SPACING            3

struct WavePoint {
  QString string;
  QColor color = WAVEFORM_DEFAULT_TEXT_COLOR;
  qreal     t;
  SUCOMPLEX point;
  SUFLOAT   angle;
  qreal     saved_t;
};

struct WaveMarker {
  QString string;
  quint64 x;
  bool below = false;
};

struct WaveVCursor {
  QString string;
  QColor color = WAVEFORM_DEFAULT_TEXT_COLOR;
  SUCOMPLEX level;
};

struct WaveACursor {
  QString string;
  QColor color = WAVEFORM_DEFAULT_TEXT_COLOR;
  SUFLOAT amplitude;
};

class WaveBuffer {
  WaveView *m_view = nullptr;

  std::vector<SUCOMPLEX> m_ownBuffer; // Only if !m_loan
  const std::vector<SUCOMPLEX> *m_buffer = nullptr; // Only if !m_ro

  const SUCOMPLEX *m_ro_data = nullptr;
  size_t           m_ro_size = 0;

  bool m_loan = false; // m_ownBuffer must be ignored
  bool m_ro   = false; // m_buffer must be ignored. Implies m_loan

  inline void
  refreshBufferCache()
  {
    if (!m_ro) {
      m_ro_data = m_buffer->data();
      m_ro_size = m_buffer->size();
    }
  }

  inline void
  updateBuffer()
  {
    if (m_view != nullptr) {
      if (m_buffer != nullptr)
        m_view->setBuffer(m_buffer);
      else
        m_view->setBuffer(m_ro_data, m_ro_size);
    }
  }

public:
  inline bool
  isLoan() const
  {
    return m_loan;
  }

  inline bool
  isReadOnly() const
  {
    return m_ro;
  }

  void operator = (const WaveBuffer &);

  WaveBuffer(WaveView *view);
  WaveBuffer(WaveView *view, const std::vector<SUCOMPLEX> *);
  WaveBuffer(WaveView *view, const SUCOMPLEX *, size_t size);

  void rebuildViews();

  bool feed(SUCOMPLEX val);
  bool feed(std::vector<SUCOMPLEX> const &);
  size_t length() const;
  const SUCOMPLEX *data() const;
  const std::vector<SUCOMPLEX> *loanedBuffer() const;
};

class Waveform : public ThrottleableWidget
{
  Q_OBJECT

  Q_PROPERTY(
      QColor backgroundColor
      READ getBackgroundColor
      WRITE setBackgroundColor
      NOTIFY backgroundColorChanged)

  Q_PROPERTY(
      QColor foregroundColor
      READ getForegroundColor
      WRITE setForegroundColor
      NOTIFY foregroundColorChanged)


  Q_PROPERTY(
      QColor axesColor
      READ getAxesColor
      WRITE setAxesColor
      NOTIFY axesColorChanged)

  Q_PROPERTY(
      QColor textColor
      READ getTextColor
      WRITE setTextColor
      NOTIFY textColorChanged)

  Q_PROPERTY(
      QColor selectionColor
      READ getSelectionColor
      WRITE setSelectionColor
      NOTIFY selectionColorChanged)

  Q_PROPERTY(
      QColor subSelectionColor
      READ getSubSelectionColor
      WRITE setSubSelectionColor
      NOTIFY subSelectionColorChanged)

  Q_PROPERTY(
      QColor envelopeColor
      READ getEnvelopeColor
      WRITE setEnvelopeColor
      NOTIFY envelopeColorChanged)

  Q_PROPERTY(
      qreal sampleRate
      READ getSampleRate
      WRITE setSampleRate
      NOTIFY sampleRateChanged)

  Q_PROPERTY(
      QString horizontalUnits
      READ getHorizontalUnits
      WRITE setHorizontalUnits
      NOTIFY horizontalUnitsChanged)

  Q_PROPERTY(
      QString horizontalAxis
      READ getHorizontalAxis
      WRITE setHorizontalAxis
      NOTIFY horizontalAxisChanged)

  Q_PROPERTY(
      QString verticalUnits
      READ getVerticalUnits
      WRITE setVerticalUnits
      NOTIFY verticalUnitsChanged)

  // Properties
  QColor m_background;
  QColor m_foreground;
  QColor m_selection;
  QColor m_subSelection;
  QColor m_envelope;
  QColor m_axes;
  QColor m_text;
  QString m_horizontalAxis = "t";
  QString m_horizontalUnits = "s";
  QString m_verticalUnits = "";

  QList<WaveMarker>      m_markerList;
  QList<WaveVCursor>     m_vCursorList;
  QList<WaveACursor>     m_aCursorList;
  QMap<qreal, WavePoint> m_pointMap;

  qreal m_oX = 0;

  bool m_periodicSelection = false;

  int m_divsPerSelection = 1;

  // State
  QSize m_geometry;
  bool m_haveGeometry = false;
  bool m_axesDrawn = false;
  bool m_waveDrawn = false;
  bool m_selUpdated = false;
  bool m_enableFeedback = true;

  QImage  m_waveform;
  QPixmap m_contentPixmap; // Data and vertical axes
  QPixmap m_axesPixmap;    // Only horizontal axes

  // Interactive state
  qreal m_savedMin;
  qreal m_savedMax;

  qint64 m_savedStart;
  qint64 m_savedEnd;

  qint64 m_clickX;
  qint64 m_clickY;

  qint64 m_clickSample;

  int  m_frequencyTextHeight;
  bool m_frequencyDragging = false;

  int  m_valueTextWidth = 0;
  bool m_valueDragging = false;

  bool m_hSelDragging = false;

  bool m_haveCursor = false;
  int  m_currMouseX = 0;

  bool m_askedToKeepView = false;

  // Limits
  WaveView   m_view;
  WaveBuffer m_data;

  // Tick length (in pixels) in which we place a time mark, starting form t0
  qreal m_hDivSamples;
  int   m_hDigits;
  qreal m_vDivUnits;
  int   m_vDigits;

  // Level height (in pixels) in which we place a level mark, from 0
  qreal m_levelHeight = 0;

  // Horizontal selection (in samples)
  bool  m_hSelection = false;
  qreal m_hSelStart = 0;
  qreal m_hSelEnd   = 0;

  // Vertical selection (int floating point units)
  bool  m_vSelection = false;
  qreal m_vSelStart  = 0;
  qreal m_vSelEnd    = 0;

  // Behavioral properties
  bool m_autoScroll = false;
  bool m_autoFitToEnvelope = true;

  void drawHorizontalAxes();
  void drawVerticalAxes();
  void drawAxes();
  void overlayMarkers(QPainter &);
  void overlayACursors(QPainter &);
  void overlayVCursors(QPainter &);
  void overlayPoints(QPainter &);
  void drawWave();
  void overlaySelection(QPainter &);
  void overlaySelectionMarkes(QPainter &);
  void recalculateDisplayData();
  void paintTriangle(QPainter &, int, int, int, QColor const &, int side = 5);
  void triggerMouseMoveHere();
  int  calcWaveViewWidth() const;

  inline bool
  somethingDirty() const
  {
    return !m_waveDrawn || !m_axesDrawn || !m_selUpdated;
  }

protected:
    //re-implemented widget event handlers
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;
    void leaveEvent(QEvent *event) override;

    bool event(QEvent *event) override;

public:
    inline bool
    isComplete() const
    {
      return m_view.isComplete();
    }

    inline bool
    isRunning() const
    {
      return m_view.isRunning();
    }

    inline SUCOMPLEX
    getDataMax() const
    {
      return m_view.getDataMax();
    }

    inline SUCOMPLEX
    getDataMin() const
    {
      return m_view.getDataMin();
    }

    inline SUCOMPLEX
    getDataMean() const
    {
      return m_view.getDataMean();
    }

    inline SUFLOAT
    getDataRMS() const
    {
      return m_view.getDataRMS();
    }

    inline qreal
    samp2t(qreal samp) const
    {
      return m_view.samp2t(samp);
    }

    inline qreal
    t2samp(qreal t) const
    {
      return m_view.t2samp(t);
    }

    inline qreal
    px2samp(qreal px) const
    {
      return m_view.px2samp(px - m_valueTextWidth);
    }

    inline qreal
    samp2px(qreal samp) const
    {
      return m_view.samp2px(samp) + m_valueTextWidth;
    }

    inline qint64
    getVerticalAxisWidth() const
    {
      return m_valueTextWidth;
    }

    inline void
    setPointMap(const QMap<qreal, WavePoint> &map)
    {
      if (!m_pointMap.empty() || !map.empty()) {
        m_pointMap = map;
        m_waveDrawn = false;
        this->invalidate();
      }
    }

    inline QMap<qreal, WavePoint>::iterator
    refreshPoint(const QMap<qreal, WavePoint>::iterator &it)
    {
      if (it->saved_t != it->t) {
        WavePoint prev = *it;
        m_pointMap.erase(it);
        prev.saved_t = prev.t;
        auto ret = m_pointMap.insert(prev.t, prev);
        m_waveDrawn = false;
        this->invalidate();
        return ret;
      } else {
        m_waveDrawn = false;
        this->invalidate();
        return it;
      }
    }

    inline void
    removePoint(const QMap<qreal, WavePoint>::iterator &it)
    {
      m_pointMap.erase(it);
      m_waveDrawn = false;
      this->invalidate();
    }

    QMap<qreal, WavePoint>::iterator
    addPoint(qreal t, SUCOMPLEX y, QColor col, SUFLOAT angle = 0)
    {
      WavePoint p;

      p.t     = t;
      p.point = y;
      p.color = col;
      p.angle = angle;
      p.saved_t = t;

      auto ret = m_pointMap.insert(p.t, p);
      m_waveDrawn = false;
      this->invalidate();
      return ret;
    }

    inline void
    setMarkerList(const QList<WaveMarker> &list)
    {
      if (!m_markerList.empty() || !list.empty()) {
        m_markerList = list;
        m_waveDrawn = false;
        this->invalidate();
      }
    }

    inline void
    setVCursorList(const QList<WaveVCursor> &list)
    {
      if (!m_vCursorList.empty() || !list.empty()) {
        m_vCursorList = list;
        m_waveDrawn = false;
        this->invalidate();
      }
    }

    inline void
    setACursorList(const QList<WaveACursor> &list)
    {
      if (!m_aCursorList.empty() || !list.empty()) {
        m_aCursorList = list;
        m_waveDrawn = false;
        this->invalidate();
      }
    }

    inline void
    setTimeStart(qreal s)
    {
      m_view.setTimeStart(s);
    }

    inline qreal
    px2t(qreal px) const
    {
      return m_view.px2t(px - m_valueTextWidth);
    }

    inline qreal
    t2px(qreal t) const
    {
      return m_view.t2px(t) + m_valueTextWidth;
    }

    inline qreal
    px2value(qreal px) const
    {
      return m_view.px2value(px);
    }

    inline qreal
    value2px(qreal val) const
    {
      return m_view.value2px(val);
    }

    inline qreal
    cast(SUCOMPLEX z) const
    {
      return m_view.cast(z);
    }

    inline bool
    computeLimits(qint64 start, qint64 end, WaveLimits &limits) const
    {
      if (m_view.isComplete()) {
        m_view.computeLimits(start, end, limits);
        return true;
      }

      return false;
    }


  const inline SUCOMPLEX *
  getData() const
  {
    return m_data.data();
  }

  size_t
  getDataLength() const
  {
    return m_data.length();
  }

  void
  setBackgroundColor(const QColor &c)
  {
    m_background = c;
    m_axesDrawn = false;
    this->invalidate();
    emit backgroundColorChanged();
  }

  const QColor &
  getBackgroundColor() const
  {
    return m_background;
  }

  void
  setAxesColor(const QColor &c)
  {
    m_axes = c;
    m_axesDrawn = false;
    this->invalidate();
    emit axesColorChanged();
  }

  const QColor &
  getAxesColor() const
  {
    return m_axes;
  }

  void
  setTextColor(const QColor &c)
  {
    m_text = c;
    m_axesDrawn = false;
    this->invalidate();
    emit textColorChanged();
  }

  const QColor &
  getTextColor() const
  {
    return m_text;
  }

  void
  setSelectionColor(const QColor &c)
  {
    m_selection = c;
    m_selUpdated = false;
    this->invalidate();
    emit textColorChanged();
  }

  const QColor &
  getSelectionColor() const
  {
    return m_selection;
  }

  void
  setSubSelectionColor(const QColor &c)
  {
    m_selection = c;
    m_selUpdated = false;
    this->invalidate();
    emit textColorChanged();
  }

  const QColor &
  getSubSelectionColor() const
  {
    return m_selection;
  }

  void
  setForegroundColor(const QColor &c)
  {
    m_foreground = c;
    m_view.setForeground(m_foreground);
    m_axesDrawn = false;
    this->invalidate();
    emit foregroundColorChanged();
  }

  const QColor &
  getForegroundColor() const
  {
    return m_foreground;
  }

  void
  setEnvelopeColor(const QColor &c)
  {
    m_envelope = c;
    m_axesDrawn = false;
    this->invalidate();
    emit envelopeColorChanged();
  }

  const QColor &
  getEnvelopeColor() const
  {
    return m_envelope;
  }

  qreal
  getSampleRate() const
  {
    return m_view.getSampleRate();
  }

  void
  setSampleRate(qreal rate)
  {
    if (rate <= 0)
      rate = static_cast<qreal>(std::numeric_limits<SUFLOAT>::epsilon());

    if (!sufreleq(rate, m_view.getSampleRate(), 1e-5f)) {
      m_view.setSampleRate(rate);
      m_axesDrawn = false;
      this->recalculateDisplayData();
      this->invalidate();
      emit sampleRateChanged();
    }
  }

  QString
  getHorizontalUnits() const
  {
    return m_horizontalUnits;
  }

  void
  setHorizontalUnits(QString units)
  {
    m_horizontalUnits = units;
    m_axesDrawn = false;
    this->invalidate();
    emit horizontalUnitsChanged();
  }

  QString
  getHorizontalAxis() const
  {
    return m_horizontalAxis;
  }

  void
  setHorizontalAxis(QString axis)
  {
    m_horizontalAxis = axis;
    m_axesDrawn = false;
    this->invalidate();
    emit horizontalAxisChanged();
  }

  QString
  getVerticalUnits() const
  {
    return m_verticalUnits;
  }

  void
  setVerticalUnits(QString units)
  {
    m_verticalUnits = units;
    m_axesDrawn = false;
    this->invalidate();
    emit verticalUnitsChanged();
  }

  void
  setDivsPerSelection(int divs)
  {
    if (divs < 1)
      divs = 1;
    m_divsPerSelection = divs;
    if (m_hSelection)
      m_selUpdated = false;
    this->invalidate();
  }


  inline qint64
  getSampleStart() const
  {
    return m_view.getSampleStart();
  }

  inline qint64
  getSampleEnd() const
  {
    return m_view.getSampleEnd();
  }

  inline qreal
  getSamplesPerPixel() const
  {
    return m_view.getSamplesPerPixel();
  }

  inline qreal
  getUnitsPerPx() const
  {
    return m_view.getUnitsPerPixel();
  }

  inline qreal
  getMax() const
  {
    return m_view.getMax();
  }

  inline qreal
  getMin() const
  {
    return m_view.getMin();
  }

  inline qreal
  getCursorTime() const
  {
    return this->px2t(m_currMouseX);
  }

  void
  setPalette(const QColor *table)
  {
    m_view.setPalette(table);

    m_waveDrawn = false;
    m_axesDrawn = false;
    this->invalidate();
  }

  void
  setOriginX(qreal origin)
  {
    m_oX = origin;
  }


  inline void
  setEnableFeedback(bool enable)
  {
    m_enableFeedback = enable;
  }

  inline qreal
  getEnvelope() const
  {
    return m_view.getEnvelope();
  }

  Waveform(QWidget *parent = nullptr);
  ~Waveform() override;

  void setData(
      const std::vector<SUCOMPLEX> *,
      bool keepView = false,
      bool flush = false);

  void setData(
      const SUCOMPLEX *,
      size_t,
      bool keepView = false,
      bool flush = false,
      bool appending = false);

  void reuseDisplayData(Waveform *);
  void draw() override;
  void paint() override;  
  void safeCancel();
  void zoomHorizontalReset(); // To show full wave or to sampPerPix = 1
  void zoomHorizontal(qint64 x, qreal amount); // Zoom at point x.
  void zoomHorizontal(qreal tStart, qreal tEnd);
  void zoomHorizontal(qint64, qint64);
  void saveHorizontal();
  void scrollHorizontal(qint64 orig, qint64 to);
  void scrollHorizontal(qint64 delta);
  void selectHorizontal(qreal orig, qreal to);
  bool getHorizontalSelectionPresent() const;
  qreal getHorizontalSelectionStart() const;
  qreal getHorizontalSelectionEnd() const;
  void setAutoScroll(bool);
  void setShowEnvelope(bool);
  void setShowPhase(bool);
  void setShowPhaseDiff(bool);
  void setShowWaveform(bool);
  void zoomVerticalReset();
  void zoomVertical(qint64 y, qreal amount);
  void zoomVertical(qreal start, qreal end);
  void saveVertical();
  void scrollVertical(qint64 orig, qint64 to);
  void scrollVertical(qint64 delta);
  void selectVertical(qint64 orig, qint64 to);
  bool getVerticalSelectionPresent() const;
  qreal getVerticalSelectionStart() const;
  qreal getVerticalSelectionEnd() const;
  void fitToEnvelope();
  void setAutoFitToEnvelope(bool);

  void setPhaseDiffOrigin(unsigned);
  void setPhaseDiffContrast(qreal);

  void resetSelection();
  void setPeriodicSelection(bool);

  void setRealComponent(bool real);
  void refreshData();

signals:
  void backgroundColorChanged();
  void foregroundColorChanged();
  void axesColorChanged();
  void textColorChanged();
  void horizontalUnitsChanged();
  void horizontalAxisChanged();
  void verticalUnitsChanged();
  void selectionColorChanged();
  void subSelectionColorChanged();
  void envelopeColorChanged();
  void sampleRateChanged();
  void axesUpdated();
  void selectionUpdated();

  void horizontalRangeChanged(qint64 start, qint64 end);
  void verticalRangeChanged(qreal min, qreal max);
  void horizontalSelectionChanged(qreal start, qreal end);
  void verticalSelectionChanged(qreal min, qreal max);
  void hoverTime(qreal);
  void waveViewChanged();
  void pointClicked(qreal, qreal, Qt::KeyboardModifiers);

  void toolTipAt(int x, int y, qreal, qreal);

public slots:
  void onWaveViewChanges();
};

#endif
