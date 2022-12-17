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

/////////////////////////////// WFHelpers //////////////////////////////////////
void
WFHelpers::drawChannelCutoff(
    QPainter &painter,
    int y,
    int x_fMin,
    int x_fMax,
    int x_fCenter,
    QColor markerColor,
    QColor cutOffColor)
{
  int h = painter.device()->height();
  QPen pen = QPen(cutOffColor);
  pen.setStyle(Qt::DashLine);
  pen.setWidth(1);

  painter.save();
  painter.setPen(pen);
  painter.setOpacity(1);

  painter.drawLine(
        x_fMin,
        y,
        x_fMin,
        h - 1);

  painter.drawLine(
        x_fMax,
        y,
        x_fMax,
        h - 1);

  pen.setColor(markerColor);
  painter.setPen(pen);

  painter.drawLine(
        x_fCenter,
        y,
        x_fCenter,
        h - 1);

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
    QColor textColor)
{
  const int padding = 3;
  int dw = x_fMax - x_fMin;

  painter.save();
  painter.setOpacity(0.3);
  painter.fillRect(x_fMin, 0, dw, h, boxColor);
  painter.setPen(markerColor);
  painter.setOpacity(1);
  painter.drawLine(x_fCenter, 0, x_fCenter, h);
  painter.restore();

  if (text.length() > 0) {
    QFont font;
    font.setBold(true);
    QFontMetrics metrics(font);
    int textHeight = metrics.height();
    int textWidth;

    painter.setFont(font);

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    textWidth = metrics.horizontalAdvance(text) + 2 * padding;
#else
    textWidth = metrics.width(text) + 2 * padding;
#endif // QT_VERSION_CHECK
    painter.save();
    painter.setOpacity(1);
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
  it = m_sortedChannels.insert(frequency + fMax, channel);

  return it;
}

bool
NamedChannelSet::isOutOfPlace(NamedChannelSetIterator it) const
{
  auto channel = it.value();
  auto key = channel->frequency + channel->highFreqCut;

  return it.key() != key;
}

NamedChannelSetIterator
NamedChannelSet::relocate(NamedChannelSetIterator it)
{
  NamedChannel *channel = *it;

  m_sortedChannels.remove(it.key(), it.value());

  it = m_sortedChannels.insert(
        channel->frequency + channel->highFreqCut,
        channel);

  return it;
}

void
NamedChannelSet::remove(NamedChannelSetIterator it)
{
  NamedChannel *channel = it.value();

  if (m_allocation.removeOne(channel)) {
    free(channel);

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
