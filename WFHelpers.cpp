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
