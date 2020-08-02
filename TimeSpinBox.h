//
//    TimeSpinBox.h: Select time periods
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
#ifndef TIMESPINBOX_H
#define TIMESPINBOX_H

#include <QWidget>
#include <QVector>

namespace Ui {
  class TimeSpinBox;
}

struct TimeSpinBoxUnit {
  QString name;
  bool timeRelative;
  qreal multiplier;

  TimeSpinBoxUnit();
  TimeSpinBoxUnit(QString name, bool timeRelative, qreal multiplier);
};

class TimeSpinBox : public QWidget
{
  Q_OBJECT

  TimeSpinBoxUnit defaultUnit;
  QVector<TimeSpinBoxUnit> units;

  qreal currSampleRate = 1;
  qreal minTime = 0;
  qreal maxTime = 60;
  qreal time;

  void adjustLimits(void);

  const TimeSpinBoxUnit *getCurrentSpinBoxUnit(void) const;

  void connectAll(void);

public:
  explicit TimeSpinBox(QWidget *parent = nullptr);
  ~TimeSpinBox();

  void addBasicTimeUnits(void);

  qreal samplesValue(void) const;
  void setSamplesValue(qreal);

  qreal timeValue(void) const;
  void setTimeValue(qreal);

  qreal sampleMin(void) const;
  qreal sampleMax(void) const;

  qreal timeMin(void) const;
  qreal timeMax(void) const;

  void setSampleMin(qreal);
  void setSampleMax(qreal);

  void setTimeMin(qreal);
  void setTimeMax(qreal);

  void clearUnits(void);
  void addUnit(QString, bool, qreal);

  void setBestUnits(bool timeRelative);

  QString getCurrentUnitName(void) const;
  bool    isCurrentUnitTimeRelative(void) const;
  qreal   getCurrentUnitMultiplier(void) const;

  void  setSampleRate(qreal);
  qreal sampleRate(void) const;

signals:
  void changed(qreal time, qreal samples);

public slots:
  void onChangeUnits(void);
  void onValueChanged(void);

private:
  Ui::TimeSpinBox *ui;

};

#endif // TIMESPINBOX_H
