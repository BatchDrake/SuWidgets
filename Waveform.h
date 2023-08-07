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

  const SUCOMPLEX *m_ro_data;
  size_t           m_ro_size;

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
  QColor background;
  QColor foreground;
  QColor selection;
  QColor subSelection;
  QColor envelope;
  QColor axes;
  QColor text;
  QString horizontalAxis = "t";
  QString horizontalUnits = "s";
  QString verticalUnits = "";

  QList<WaveMarker>      markerList;
  QList<WaveVCursor>     vCursorList;
  QList<WaveACursor>     aCursorList;
  QMap<qreal, WavePoint> pointMap;

  qreal oX = 0;

  bool periodicSelection = false;

  int divsPerSelection = 1;

  // State
  QSize geometry;
  bool haveGeometry = false;
  bool axesDrawn = false;
  bool waveDrawn = false;
  bool selUpdated = false;
  bool enableFeedback = true;

  QImage  waveform;
  QPixmap contentPixmap; // Data and vertical axes
  QPixmap axesPixmap;    // Only horizontal axes

  // Interactive state
  qreal savedMin;
  qreal savedMax;

  qint64 savedStart;
  qint64 savedEnd;

  qint64 clickX;
  qint64 clickY;

  qint64 clickSample;

  int  frequencyTextHeight;
  bool frequencyDragging = false;

  int  valueTextWidth = 0;
  bool valueDragging = false;

  bool hSelDragging = false;

  bool haveCursor = false;
  int  currMouseX = 0;

  bool askedToKeepView = false;

  // Limits
  WaveView   m_view;
  WaveBuffer m_data;

  // Tick length (in pixels) in which we place a time mark, starting form t0
  qreal hDivSamples;
  int   hDigits;
  qreal vDivUnits;
  int   vDigits;

  // Level height (in pixels) in which we place a level mark, from 0
  qreal levelHeight = 0;

  // Horizontal selection (in samples)
  bool  hSelection = false;
  qreal hSelStart = 0;
  qreal hSelEnd   = 0;

  // Vertical selection (int floating point units)
  bool  vSelection = false;
  qreal vSelStart  = 0;
  qreal vSelEnd    = 0;

  // Behavioral properties
  bool autoScroll = false;
  bool autoFitToEnvelope = true;

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
    return !this->waveDrawn || !this->axesDrawn || !this->selUpdated;
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
      return m_view.px2samp(px - this->valueTextWidth);
    }

    inline qreal
    samp2px(qreal samp) const
    {
      return m_view.samp2px(samp) + this->valueTextWidth;
    }

    inline qint64
    getVerticalAxisWidth() const
    {
      return this->valueTextWidth;
    }

    inline void
    setPointMap(const QMap<qreal, WavePoint> &map)
    {
      if (!this->pointMap.empty() || !map.empty()) {
        this->pointMap = map;
        this->waveDrawn = false;
        this->invalidate();
      }
    }

    inline QMap<qreal, WavePoint>::iterator
    refreshPoint(const QMap<qreal, WavePoint>::iterator &it)
    {
      if (it->saved_t != it->t) {
        WavePoint prev = *it;
        this->pointMap.erase(it);
        prev.saved_t = prev.t;
        auto ret = this->pointMap.insert(prev.t, prev);
        this->waveDrawn = false;
        this->invalidate();
        return ret;
      } else {
        this->waveDrawn = false;
        this->invalidate();
        return it;
      }
    }

    inline void
    removePoint(const QMap<qreal, WavePoint>::iterator &it)
    {
      this->pointMap.erase(it);
      this->waveDrawn = false;
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

      auto ret = this->pointMap.insert(p.t, p);
      this->waveDrawn = false;
      this->invalidate();
      return ret;
    }

    inline void
    setMarkerList(const QList<WaveMarker> &list)
    {
      if (!this->markerList.empty() || !list.empty()) {
        this->markerList = list;
        this->waveDrawn = false;
        this->invalidate();
      }
    }

    inline void
    setVCursorList(const QList<WaveVCursor> &list)
    {
      if (!this->vCursorList.empty() || !list.empty()) {
        this->vCursorList = list;
        this->waveDrawn = false;
        this->invalidate();
      }
    }

    inline void
    setACursorList(const QList<WaveACursor> &list)
    {
      if (!this->aCursorList.empty() || !list.empty()) {
        this->aCursorList = list;
        this->waveDrawn = false;
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
      return m_view.px2t(px - this->valueTextWidth);
    }

    inline qreal
    t2px(qreal t) const
    {
      return m_view.t2px(t) + this->valueTextWidth;
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
    this->background = c;
    this->axesDrawn = false;
    this->invalidate();
    emit backgroundColorChanged();
  }

  const QColor &
  getBackgroundColor() const
  {
    return this->background;
  }

  void
  setAxesColor(const QColor &c)
  {
    this->axes = c;
    this->axesDrawn = false;
    this->invalidate();
    emit axesColorChanged();
  }

  const QColor &
  getAxesColor() const
  {
    return this->axes;
  }

  void
  setTextColor(const QColor &c)
  {
    this->text = c;
    this->axesDrawn = false;
    this->invalidate();
    emit textColorChanged();
  }

  const QColor &
  getTextColor() const
  {
    return this->text;
  }

  void
  setSelectionColor(const QColor &c)
  {
    this->selection = c;
    this->selUpdated = false;
    this->invalidate();
    emit textColorChanged();
  }

  const QColor &
  getSelectionColor() const
  {
    return this->selection;
  }

  void
  setSubSelectionColor(const QColor &c)
  {
    this->selection = c;
    this->selUpdated = false;
    this->invalidate();
    emit textColorChanged();
  }

  const QColor &
  getSubSelectionColor() const
  {
    return this->selection;
  }

  void
  setForegroundColor(const QColor &c)
  {
    this->foreground = c;
    m_view.setForeground(this->foreground);
    this->axesDrawn = false;
    this->invalidate();
    emit foregroundColorChanged();
  }

  const QColor &
  getForegroundColor() const
  {
    return this->foreground;
  }

  void
  setEnvelopeColor(const QColor &c)
  {
    this->envelope = c;
    this->axesDrawn = false;
    this->invalidate();
    emit envelopeColorChanged();
  }

  const QColor &
  getEnvelopeColor() const
  {
    return this->envelope;
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
      this->axesDrawn = false;
      this->recalculateDisplayData();
      this->invalidate();
      emit sampleRateChanged();
    }
  }

  QString
  getHorizontalUnits() const
  {
    return this->horizontalUnits;
  }

  void
  setHorizontalUnits(QString units)
  {
    this->horizontalUnits = units;
    this->axesDrawn = false;
    this->invalidate();
    emit horizontalUnitsChanged();
  }

  QString
  getHorizontalAxis() const
  {
    return this->horizontalAxis;
  }

  void
  setHorizontalAxis(QString axis)
  {
    this->horizontalAxis = axis;
    this->axesDrawn = false;
    this->invalidate();
    emit horizontalAxisChanged();
  }

  QString
  getVerticalUnits() const
  {
    return this->verticalUnits;
  }

  void
  setVerticalUnits(QString units)
  {
    this->verticalUnits = units;
    this->axesDrawn = false;
    this->invalidate();
    emit verticalUnitsChanged();
  }

  void
  setDivsPerSelection(int divs)
  {
    if (divs < 1)
      divs = 1;
    this->divsPerSelection = divs;
    if (this->hSelection)
      this->selUpdated = false;
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
    return this->px2t(this->currMouseX);
  }

  void
  setPalette(const QColor *table)
  {
    m_view.setPalette(table);

    this->waveDrawn = false;
    this->axesDrawn = false;
    this->invalidate();
  }

  void
  setOriginX(qreal origin)
  {
    this->oX = origin;
  }


  inline void
  setEnableFeedback(bool enable)
  {
    this->enableFeedback = enable;
  }

  inline qreal
  getEnvelope() const
  {
    return m_view.getEnvelope();
  }

  Waveform(QWidget *parent = nullptr);
  ~Waveform();

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
