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
  bool timeRelative = false;
  qreal multiplier = 1;

  TimeSpinBoxUnit();
  TimeSpinBoxUnit(QString name, bool timeRelative, qreal multiplier);
};

class TimeSpinBox : public QWidget
{
  Q_OBJECT

  TimeSpinBoxUnit          m_defaultUnit;
  QVector<TimeSpinBoxUnit> m_units;

  qreal m_currSampleRate = 1;
  qreal m_minTime        = 0;
  qreal m_maxTime        = 60;
  qreal m_time           = 0;

  void adjustLimits();

  const TimeSpinBoxUnit *getCurrentSpinBoxUnit() const;

  void connectAll();

public:
  explicit TimeSpinBox(QWidget *parent = nullptr);
  ~TimeSpinBox();

  void addBasicTimeUnits();

  qreal samplesValue() const;
  void setSamplesValue(qreal);

  qreal timeValue() const;
  void setTimeValue(qreal);

  qreal sampleMin() const;
  qreal sampleMax() const;

  qreal timeMin() const;
  qreal timeMax() const;

  void setSampleMin(qreal);
  void setSampleMax(qreal);

  void setDecimals(int);

  void setTimeMin(qreal);
  void setTimeMax(qreal);

  void clearUnits();
  void addUnit(QString, bool, qreal);

  void setBestUnits(bool timeRelative);

  QString getCurrentUnitName() const;
  bool    isCurrentUnitTimeRelative() const;
  qreal   getCurrentUnitMultiplier() const;

  void  setSampleRate(qreal);
  qreal sampleRate() const;

signals:
  void changed(qreal time, qreal samples);

public slots:
  void onChangeUnits();
  void onValueChanged();

private:
  Ui::TimeSpinBox *m_ui;

};

#endif // TIMESPINBOX_H
