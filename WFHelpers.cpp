//
//    WFHelpers.cpp: Waterfall helper classes
//    Copyright (C) 2021 Gonzalo José Carracedo Carballal
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

#include "WFHelpers.h"
#include <cmath>
#include <cstdlib>

#ifdef _MSC_VER
#include <Windows.h>
#include <cstdint>

int
gettimeofday(struct timeval * tp, struct timezone * tzp)
{
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime( &system_time );
    SystemTimeToFileTime( &system_time, &file_time );
    time =  ((uint64_t)file_time.dwLowDateTime )      ;
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec  = (long) ((time - EPOCH) / 10000000L);
    tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
    return 0;
}

#endif // _MSC_VER

#define isSamePixel(x1, x2) (abs(x1 - x2) <= 1)

/////////////////////////////// WFHelpers //////////////////////////////////////
void
WFHelpers::drawChannelCutoff(
    QPainter &painter,
    int y,
    int x_fMin,
    int x_fMax,
    int x_fCenter,
    QColor markerColor,
    QColor cutOffColor,
    bool centralLine)
{
  int h = painter.device()->height();
  QPen pen = QPen(cutOffColor);
  pen.setStyle(Qt::DashLine);
  pen.setWidth(1);

  painter.save();
  painter.setPen(pen);
  painter.setOpacity(1);

  if (centralLine && !isSamePixel(x_fCenter, x_fMin))
    painter.drawLine(
          x_fMin,
          y,
          x_fMin,
          h - 1);

  if (centralLine && !isSamePixel(x_fCenter, x_fMax))
    painter.drawLine(
          x_fMax,
          y,
          x_fMax,
          h - 1);

  if (centralLine) {
    pen.setColor(markerColor);
    painter.setPen(pen);

    painter.drawLine(
          x_fCenter,
          y,
          x_fCenter,
          h - 1);
  }

  if (!isSamePixel(x_fMin, x_fMax)) {
    pen.setColor(cutOffColor);
    painter.setPen(pen);

    painter.setOpacity(.5);
    int x_extraLine = -1;
    if (x_fMin == x_fCenter)
      x_extraLine = 2 * x_fCenter - x_fMax;
    else if (x_fMax == x_fCenter)
      x_extraLine = 2 * x_fCenter - x_fMin;

    painter.drawLine(x_extraLine, y, x_extraLine, h - 1);
  }

  painter.restore();
}

void
WFHelpers::drawLineWithArrow(
    QPainter &painter,
    QPointF start,
    QPointF end,
    qreal arrowSize)
{
  QLineF line(end, start);
  double angle = std::atan2(-line.dy(), line.dx());
  QPointF arrowP1 =
      line.p1() + QPointF(sin(angle + M_PI / 3) * arrowSize,
      cos(angle + M_PI / 3) * arrowSize);
  QPointF arrowP2 =
      line.p1() + QPointF(sin(angle + M_PI - M_PI / 3) * arrowSize,
      cos(angle + M_PI - M_PI / 3) * arrowSize);
  QPolygonF arrowHead;
  QPen pen = painter.pen();

  pen.setStyle(Qt::SolidLine);
  painter.save();

  arrowHead.clear();
  arrowHead << line.p1() << arrowP1 << arrowP2;
  painter.drawLine(line);

  painter.setPen(pen);
  painter.setBrush(QBrush(pen.color()));

  painter.drawPolygon(arrowHead);

  painter.restore();
}


void
WFHelpers::drawChannelBox(
    QPainter &painter,
    int h,
    int x_fMin,
    int x_fMax,
    int x_fCenter,
    QColor boxColor,
    QColor markerColor,
    QString text,
    QColor textColor,
    int horizontalOffset,
    int verticalOffset)
{
  const int padding = 3;
  QPen borderPen = QPen(boxColor, 1, Qt::DashLine);
  int dw = x_fMax - x_fMin;
  bool bandLike = horizontalOffset >= 0;
  int y = verticalOffset;

  // Paint box
  painter.save();
  painter.setOpacity(0.3);
  painter.fillRect(x_fMin, y, dw, h, boxColor);

  // Draw marker
  if (!bandLike) {
    painter.setPen(markerColor);
    painter.setOpacity(1);
    painter.drawLine(x_fCenter, y, x_fCenter, h);
  }

  // Draw border
  painter.setOpacity(1);
  painter.setPen(borderPen);

  if (bandLike || !isSamePixel(x_fCenter, x_fMin))
    painter.drawLine(x_fMin, y, x_fMin, h);

  if (bandLike || !isSamePixel(x_fCenter, x_fMax))
    painter.drawLine(x_fMax, y, x_fMax, h);

  if (y > 0 && !bandLike)
    painter.drawLine(x_fMin, y, x_fMax, y);

  if (!isSamePixel(x_fMin, x_fMax)) {
    painter.setOpacity(.5);
    int x_extraLine = -1;
    if (x_fMin == x_fCenter)
      x_extraLine = 2 * x_fCenter - x_fMax;
    else if (x_fMax == x_fCenter)
      x_extraLine = 2 * x_fCenter - x_fMin;

    painter.drawLine(x_extraLine, y, x_extraLine, h);
  }

  painter.restore();

  // Draw text (if provided)
  if (text.length() > 0) {
    QFont font;
    font.setBold(!bandLike);
    QFontMetrics metrics(font);
    int textHeight = metrics.height();
    int textWidth;

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0l)
    textWidth = metrics.horizontalAdvance(text) + 2 * padding;
#else
    textWidth = metrics.width(text) + 2 * padding;
#endif // QT_VERSION_CHECK

    painter.save();
    painter.setFont(font);

    painter.translate(0, y);
    h -= y;
    painter.setOpacity(1);

    if (bandLike) {
      int x_start_left  = x_fCenter - textWidth / 2 - textHeight / 2;
      int x_start_right = x_fCenter + textWidth / 2;
      int ydispl = textWidth > dw ? textHeight / 2 : 0;

      painter.setPen(borderPen);

      drawLineWithArrow(
            painter,
            QPointF(x_start_left, horizontalOffset + ydispl),
            QPointF(x_fMin, horizontalOffset + ydispl));
      drawLineWithArrow(
            painter,
            QPointF(x_start_right, horizontalOffset + ydispl),
            QPointF(x_fMax, horizontalOffset + ydispl));

      painter.setPen(textColor);
      painter.drawText(
            x_fCenter - textWidth / 2,
            horizontalOffset + textHeight / 4,
            text);
    } else {
      painter.fillRect(
            x_fCenter - textHeight / 2,
            (h - textWidth) / 2,
            textHeight,
            textWidth,
            markerColor);
      painter.setPen(markerColor);
      painter.setBrush(QBrush(markerColor));
      painter.drawChord(
            x_fCenter - textHeight / 2,
            (h - textWidth) / 2 - textHeight / 2,
            textHeight,
            textHeight,
            0,
            180 * 16);

      painter.drawChord(
            x_fCenter - textHeight / 2,
            (h + textWidth) / 2 - textHeight / 2,
            textHeight,
            textHeight,
            180 * 16,
            180 * 16);


      painter.setPen(textColor);

      painter.translate(x_fCenter, (h + textWidth) / 2);
      painter.rotate(-90);
      painter.drawText(padding, textHeight / 3, text);
    }

    painter.restore();
  }
}

////////////////////////// BookmarkSource //////////////////////////////////////
BookmarkSource::~BookmarkSource()
{

}

/////////////////////////// FrequencyBand //////////////////////////////////////
FrequencyAllocationTable::FrequencyAllocationTable()
{

}

FrequencyAllocationTable::FrequencyAllocationTable(std::string const &name)
{
  this->name = name;
}

void
FrequencyAllocationTable::pushBand(FrequencyBand const &band)
{
  this->allocation[band.min] = band;
}

void
FrequencyAllocationTable::pushBand(qint64 min, qint64 max, std::string const &desc)
{
  FrequencyBand band;

  band.min = min;
  band.max = max;
  band.primary = desc;
  band.color = QColor::fromRgb(255, 0, 0);

  this->pushBand(band);
}

FrequencyBandIterator
FrequencyAllocationTable::cbegin(void) const
{
  return this->allocation.cbegin();
}

FrequencyBandIterator
FrequencyAllocationTable::cend(void) const
{
  return this->allocation.cend();
}

FrequencyBandIterator
FrequencyAllocationTable::find(qint64 freq) const
{
  if (this->allocation.size() == 0)
    return this->allocation.cend();

  auto lower = this->allocation.lower_bound(freq);

  if (lower == this->cend()) // If none found, return the last one.
      return std::prev(lower);

  if (lower == this->cbegin())
      return lower;

  // Check which one is closest.
  auto previous = std::prev(lower);
  if ((freq - previous->first) < (lower->first - freq))
      return previous;

  return lower;
}

//////////////////////////// NamedChannelSet ///////////////////////////////////
NamedChannelSetIterator
NamedChannelSet::addChannel(
    QString name,
    qint64 frequency,
    qint32 fMin,
    qint32 fMax,
    QColor boxColor,
    QColor markerColor,
    QColor cutOffColor)
{
  NamedChannelSetIterator it = m_sortedChannels.cend();
  NamedChannel *channel = new NamedChannel();

  channel->name        = name;
  channel->frequency   = frequency;
  channel->lowFreqCut  = fMin;
  channel->highFreqCut = fMax;
  channel->boxColor    = boxColor;
  channel->markerColor = markerColor;
  channel->cutOffColor = cutOffColor;

  m_allocation.append(channel);
  it = m_sortedChannels.insert(frequency + fMin, channel);

  return it;
}

bool
NamedChannelSet::isOutOfPlace(NamedChannelSetIterator it) const
{
  auto channel = it.value();
  auto key = channel->frequency + channel->lowFreqCut;

  return it.key() != key;
}

NamedChannelSetIterator
NamedChannelSet::relocate(NamedChannelSetIterator it)
{
  NamedChannel *channel = *it;

  m_sortedChannels.remove(it.key(), it.value());

  it = m_sortedChannels.insert(
        channel->frequency + channel->lowFreqCut,
        channel);

  return it;
}

void
NamedChannelSet::remove(NamedChannelSetIterator it)
{
  NamedChannel *channel = it.value();

  if (m_allocation.removeOne(channel)) {
    delete channel;

    m_sortedChannels.remove(it.key(), it.value());
  }
}

NamedChannelSetIterator
NamedChannelSet::cbegin() const
{
  return m_sortedChannels.cbegin();
}

NamedChannelSetIterator
NamedChannelSet::cend() const
{
  return m_sortedChannels.cend();
}

NamedChannelSetIterator
NamedChannelSet::find(qint64 freq)
{
  return m_sortedChannels.upperBound(freq);
}

NamedChannelSet::~NamedChannelSet()
{
  for (auto p : m_allocation)
    delete p;
}
