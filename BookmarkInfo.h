#ifndef BOOKMARK_INFO_H
#define BOOKMARK_INFO_H

#include <QList>
#include <QString>
#include <QColor>

struct BookmarkInfo {
  QString name; ///< name of bookmark
  qint64 frequency = 0; ///< [Hz] center frequency
  QColor color; ///< color of bookmark
  qint32 lowFreqCut = 0; ///< [Hz] offset from frequency (mostly negative)
  qint32 highFreqCut = 0; ///< [Hz] offset from frequency (moslty positive)
  QString modulation; ///< modulation like "AM", "FM", "USB", "LSB"

  qint32 bandwidth() { return highFreqCut - lowFreqCut; }
};

#endif