//
//    TimeSpinBox.cpp: Select time periods
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
#include "TimeSpinBox.h"
#include "ui_TimeSpinBox.h"
#include <cmath>

TimeSpinBoxUnit::TimeSpinBoxUnit(
    QString name,
    bool timeRelative,
    qreal multiplier) :
  name(name),
  timeRelative(timeRelative),
  multiplier(multiplier)
{

}

TimeSpinBoxUnit::TimeSpinBoxUnit()
{
  name = "(no units)";
  timeRelative = false;
  multiplier = 1;
}

TimeSpinBox::TimeSpinBox(QWidget *parent) :
  QWidget(parent),
  m_ui(new Ui::TimeSpinBox)
{
  m_ui->setupUi(this);

  clearUnits();
  addBasicTimeUnits();

  m_ui->valueSpin->setKeyboardTracking(false);

  connectAll();
}

TimeSpinBox::~TimeSpinBox()
{
  delete m_ui;
}

void
TimeSpinBox::connectAll()
{
  connect(
        m_ui->unitCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onChangeUnits()));

  connect(
        m_ui->valueSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onValueChanged()));
}

void
TimeSpinBox::addBasicTimeUnits()
{
  addUnit("s", true, 1);
  addUnit("ms", true, 1e-3);
  addUnit("µs", true, 1e-6);
  addUnit("ns", true, 1e-9);
}

void
TimeSpinBox::adjustLimits()
{
  const TimeSpinBoxUnit *units = getCurrentSpinBoxUnit();
  qreal min, max;
  qreal currentTimeVal = timeValue();

  min = m_minTime / units->multiplier;
  max = m_maxTime / units->multiplier;

  if (!units->timeRelative) {
    min *= m_currSampleRate;
    max *= m_currSampleRate;
  }

  m_ui->valueSpin->setMinimum(min);
  m_ui->valueSpin->setMaximum(max);

  setTimeValue(currentTimeVal);
}

qreal
TimeSpinBox::sampleMin() const
{
  return m_minTime / m_currSampleRate;
}

qreal
TimeSpinBox::sampleMax() const
{
return m_maxTime / m_currSampleRate;
}

qreal
TimeSpinBox::timeMin() const
{
  return m_minTime;
}

qreal
TimeSpinBox::timeMax() const
{
  return m_maxTime;
}

void
TimeSpinBox::setSampleMin(qreal value)
{
  m_minTime = value / m_currSampleRate;
  adjustLimits();
}

void
TimeSpinBox::setSampleMax(qreal value)
{
  m_maxTime = value / m_currSampleRate;
  adjustLimits();
}

void
TimeSpinBox::setDecimals(int prec)
{
  m_ui->valueSpin->setDecimals(prec);
}

void
TimeSpinBox::setTimeMin(qreal value)
{
  m_minTime = value;
  adjustLimits();
}

void
TimeSpinBox::setTimeMax(qreal value)
{
  m_maxTime = value;
  adjustLimits();
}

const TimeSpinBoxUnit *
TimeSpinBox::getCurrentSpinBoxUnit() const
{
  int currentIndex = m_ui->unitCombo->currentIndex();

  if (currentIndex < 0 || currentIndex >= m_units.size())
    return &m_defaultUnit;

  return &m_units[currentIndex];
}

qreal
TimeSpinBox::samplesValue() const
{
  return m_time * m_currSampleRate;
}

void
TimeSpinBox::setSamplesValue(qreal value)
{
  const TimeSpinBoxUnit *units = getCurrentSpinBoxUnit();

  m_time = value / m_currSampleRate;

  if (units->timeRelative)
    value /= m_currSampleRate;

  m_ui->valueSpin->setValue(value / units->multiplier);
}

qreal
TimeSpinBox::timeValue() const
{
  return m_time;
}

void
TimeSpinBox::setTimeValue(qreal value)
{
  const TimeSpinBoxUnit *units = getCurrentSpinBoxUnit();

  m_time = value;

  if (!units->timeRelative)
    value *= m_currSampleRate;

  m_ui->valueSpin->setValue(value / units->multiplier);
}

void
TimeSpinBox::clearUnits()
{
  m_units.clear();
  m_ui->unitCombo->clear();
}

void
TimeSpinBox::addUnit(QString name, bool timeRelative, qreal multiplier)
{
  m_units.push_back(TimeSpinBoxUnit(name, timeRelative, multiplier));
  m_ui->unitCombo->addItem(name);
}

QString
TimeSpinBox::getCurrentUnitName() const
{
  return getCurrentSpinBoxUnit()->name;
}

bool
TimeSpinBox::isCurrentUnitTimeRelative() const
{
  return getCurrentSpinBoxUnit()->timeRelative;
}

qreal
TimeSpinBox::getCurrentUnitMultiplier() const
{
  return getCurrentSpinBoxUnit()->multiplier;
}

void
TimeSpinBox::setSampleRate(qreal sampleRate)
{
  if (sampleRate > 0) {
    m_currSampleRate = sampleRate;
    adjustLimits();
  }
}

qreal
TimeSpinBox::sampleRate() const
{
  return m_currSampleRate;
}

void
TimeSpinBox::setBestUnits(bool timeRelative)
{
  qreal absValue = std::abs(
        timeRelative ? timeValue() : samplesValue());

  if (absValue > 0) {
    int bestUnitNdx = -1;
    qreal bestDigitCount = 0;
    qreal oldTimeValue = timeValue();
    for (int i = 0; i < m_units.size(); ++i) {
      if (m_units[i].timeRelative == timeRelative) {
        qreal digitCount = std::log10(absValue / m_units[i].multiplier);

        if (digitCount >= 0) {
          if (bestUnitNdx == -1 || digitCount < bestDigitCount) {
            bestUnitNdx = i;
            bestDigitCount = digitCount;
          }
        }
      }
    }

    if (bestUnitNdx != -1) {
      m_ui->unitCombo->setCurrentIndex(bestUnitNdx);
      adjustLimits();
      setTimeValue(oldTimeValue);
    }
  }
}

/////////////////////////////////// Slots //////////////////////////////////////
void
TimeSpinBox::onChangeUnits()
{
  qreal oldTimeValue = timeValue();
  adjustLimits();
  setTimeValue(oldTimeValue);
}

void
TimeSpinBox::onValueChanged()
{
  const TimeSpinBoxUnit *units = getCurrentSpinBoxUnit();
  qreal value = m_ui->valueSpin->value();
  qreal current = m_time / units->multiplier;

  if (!units->timeRelative)
    current /= m_currSampleRate;

  if (std::abs(value - current) >= 1e-2) {
    if (!units->timeRelative)
      value /= m_currSampleRate;

    m_time = value * units->multiplier;

    emit changed(timeValue(), samplesValue());
  }
}

