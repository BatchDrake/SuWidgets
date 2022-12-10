//
//    WFHelpers.h: Waterfall helper classes
//    Copyright (C) 2021 Jaroslav Safka
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

#ifndef WFHELPERS_H
#define WFHELPERS_H

#include <QList>
#include <QHash>
#include <QMultiMap>
#include <QString>
#include <QColor>
#include <map>
#include <QPainter>

#define CUR_CUT_DELTA 5		//cursor capture delta in pixels

#define FFT_MIN_DB     -120.f
#define FFT_MAX_DB      40.f

// Colors of type QRgb in 0xAARRGGBB format (unsigned int)
#define PLOTTER_BGD_COLOR           0xFF1F1D1D
#define PLOTTER_GRID_COLOR          0xFF444242
#define PLOTTER_TEXT_COLOR          0xFFDADADA
#define PLOTTER_CENTER_LINE_COLOR   0xFF788296
#define PLOTTER_FILTER_LINE_COLOR   0xFFFF7171
#define PLOTTER_FILTER_BOX_COLOR    0xFFA0A0A4
// FIXME: Should cache the QColors also

#define HORZ_DIVS_MAX 12    //50
#define VERT_DIVS_MIN 5
#define MAX_SCREENSIZE 16384

#define PEAK_CLICK_MAX_H_DISTANCE 10 //Maximum horizontal distance of clicked point from peak
#define PEAK_CLICK_MAX_V_DISTANCE 20 //Maximum vertical distance of clicked point from peak
#define PEAK_H_TOLERANCE 2
#define MINIMUM_REFRESH_RATE      25

struct BookmarkInfo {
  QString name; ///< name of bookmark
  qint64 frequency; ///< [Hz] center frequency
  QColor color; ///< color of bookmark
  qint32 lowFreqCut; ///< [Hz] offset from frequency (mostly negative)
  qint32 highFreqCut; ///< [Hz] offset from frequency (moslty positive)
  QString modulation; ///< modulation like "AM", "FM", "USB", "LSB"

  qint32 bandwidth() { return highFreqCut - lowFreqCut; }

  BookmarkInfo()
    : name(),
    frequency(0),
    color(),
    lowFreqCut(0),
    highFreqCut(0),
    modulation()
  { }

  // note: copy constructor can be used default
};

class BookmarkSource {
  public:
    virtual ~BookmarkSource();
    virtual QList<BookmarkInfo> getBookmarksInRange(qint64, qint64) = 0;
};

struct FrequencyBand {
  qint64 min;
  qint64 max;
  std::string primary;
  std::string secondary;
  std::string footnotes;
  QColor color;
};

struct TimeStamp {
  int counter;
  QString timeStampText;
  bool marker = false;
};

typedef std::map<qint64, FrequencyBand>::const_iterator FrequencyBandIterator;

class FrequencyAllocationTable {
  std::string name;
  std::map<qint64, FrequencyBand> allocation;

public:
  FrequencyAllocationTable();
  FrequencyAllocationTable(std::string const &name);

  void
  setName(std::string const &name)
  {
    this->name = name;
  }
  std::string const &
  getName(void) const
  {
    return this->name;
  }

  void pushBand(FrequencyBand const &);
  void pushBand(qint64, qint64, std::string const &);

  FrequencyBandIterator cbegin(void) const;
  FrequencyBandIterator cend(void) const;

  FrequencyBandIterator find(qint64 freq) const;
};

struct NamedChannel {
  QString name;
  qint64  frequency;    // Center frequency
  qint32  lowFreqCut;   // Low frequency cut (with respect to frequency)
  qint32  highFreqCut;  // Upper frequency cut (with respect to frequency)

  QColor  boxColor;
  QColor  markerColor;
  QColor  cutOffColor;
};

typedef QMultiMap<qint64, NamedChannel *>::const_iterator NamedChannelSetIterator;

class NamedChannelSet {
  QList<NamedChannel *> m_allocation;
  QMultiMap<qint64, NamedChannel *> m_sortedChannels;

public:
  NamedChannelSetIterator addChannel(
      QString name,
      qint64 frequency,
      qint32 fMin,
      qint32 fMax,
      QColor boxColor,
      QColor markerColor,
      QColor cutOffColor);

  NamedChannelSetIterator relocate(NamedChannelSetIterator);
  void remove(NamedChannelSetIterator);

  NamedChannelSetIterator cbegin() const;
  NamedChannelSetIterator cend() const;

  NamedChannelSetIterator find(qint64);

  ~NamedChannelSet();
};


#ifndef _MSC_VER
# include <sys/time.h>
#else
int gettimeofday(struct timeval * tp, struct timezone * tzp);
#endif // _MSC_VER

static inline bool val_is_out_of_range(float val, float min, float max)
{
    return (val < min || val > max);
}

static inline bool out_of_range(float min, float max)
{
    return (val_is_out_of_range(min, FFT_MIN_DB, FFT_MAX_DB) ||
            val_is_out_of_range(max, FFT_MIN_DB, FFT_MAX_DB) ||
            max < min + 10.f);
}

/** Current time in milliseconds since Epoch */
static inline quint64 time_ms(void)
{
    struct timeval  tval;

    gettimeofday(&tval, nullptr);

    return 1e3 * tval.tv_sec + 1e-3 * tval.tv_usec;
}

class WFHelpers {
  public:
    static void drawChannelCutoff(
        QPainter &painter,
        int h,
        int x_fMin,
        int x_fMax,
        int x_fCenter,
        QColor markerColor,
        QColor cutOffColor);

    static void drawChannelBox(
        QPainter &painter,
        int h,
        int x_fMin,
        int x_fMax,
        int x_fCenter,
        QColor boxColor,
        QColor markerColor,
        QString text = "",
        QColor textColor = QColor());
};

#endif // WFHELPERS_H

