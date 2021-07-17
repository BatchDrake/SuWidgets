//
//    BookmarkInfo.h: Bookmark info structure
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

#ifndef BOOKMARK_INFO_H
#define BOOKMARK_INFO_H

#include <QList>
#include <QString>
#include <QColor>

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

#endif