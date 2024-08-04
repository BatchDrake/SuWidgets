//
//    filename: description
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
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
#ifndef WAVEWORKER_H
#define WAVEWORKER_H

#include <QMutex>
#include <QWaitCondition>

#include "WaveViewTree.h"

class WaveWorker : public QObject {
  Q_OBJECT

  SUSCOUNT m_since = 0;
  WaveViewTree *m_owner = nullptr;
  bool m_cancelFlag = false;
  bool m_running = true;

  // Used to wait for completion
  QMutex m_mutex;
  QWaitCondition m_finishedCondition;

  // Private methods
  void buildNextView(
      WaveViewTree::iterator,
      SUSCOUNT start,
      SUSCOUNT end,
      SUFLOAT wEnd);
  void build(SUSCOUNT start, SUSCOUNT end);

public:
  WaveWorker(WaveViewTree *, SUSCOUNT since, QObject *parent = nullptr);
  ~WaveWorker() override;

  inline bool running() const { return m_running; }
  inline bool isCancelled() const { return m_cancelFlag; }

public slots:
  void run(void);
  void cancel(void);
  void wait(void);

signals:
  void finished(void);
  void progress(quint64, quint64);
  void cancelled(void);
};

#endif // WAVEWORKER_H
