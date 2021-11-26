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
  this->name = "(no units)";
  this->timeRelative = false;
  this->multiplier = 1;
}

TimeSpinBox::TimeSpinBox(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::TimeSpinBox)
{
  ui->setupUi(this);

  this->clearUnits();
  this->addBasicTimeUnits();

  this->connectAll();
}

TimeSpinBox::~TimeSpinBox()
{
  delete ui;
}

void
TimeSpinBox::connectAll(void)
{
  connect(
        this->ui->unitCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onChangeUnits()));

  connect(
        this->ui->valueSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onValueChanged()));
}

void
TimeSpinBox::addBasicTimeUnits(void)
{
  this->addUnit("s", true, 1);
  this->addUnit("ms", true, 1e-3);
  this->addUnit("µs", true, 1e-6);
  this->addUnit("ns", true, 1e-9);
}

void
TimeSpinBox::adjustLimits(void)
{
  const TimeSpinBoxUnit *units = this->getCurrentSpinBoxUnit();
  qreal min, max;
  qreal currentTimeVal = this->timeValue();

  min = minTime / units->multiplier;
  max = maxTime / units->multiplier;

  if (!units->timeRelative) {
    min *= this->currSampleRate;
    max *= this->currSampleRate;
  }

  this->ui->valueSpin->setMinimum(min);
  this->ui->valueSpin->setMaximum(max);

  this->setTimeValue(currentTimeVal);
}

qreal
TimeSpinBox::sampleMin(void) const
{
  return this->minTime / this->currSampleRate;
}

qreal
TimeSpinBox::sampleMax(void) const
{
return this->maxTime / this->currSampleRate;
}

qreal
TimeSpinBox::timeMin(void) const
{
  return this->minTime;
}

qreal
TimeSpinBox::timeMax(void) const
{
  return this->maxTime;
}

void
TimeSpinBox::setSampleMin(qreal value)
{
  this->minTime = value / this->currSampleRate;
  this->adjustLimits();
}

void
TimeSpinBox::setSampleMax(qreal value)
{
  this->maxTime = value / this->currSampleRate;
  this->adjustLimits();
}

void
TimeSpinBox::setTimeMin(qreal value)
{
  this->minTime = value;
  this->adjustLimits();
}

void
TimeSpinBox::setTimeMax(qreal value)
{
  this->maxTime = value;
  this->adjustLimits();
}

const TimeSpinBoxUnit *
TimeSpinBox::getCurrentSpinBoxUnit(void) const
{
  int currentIndex = this->ui->unitCombo->currentIndex();

  if (currentIndex < 0 || currentIndex >= this->units.size())
    return &this->defaultUnit;

  return &this->units[currentIndex];
}

qreal
TimeSpinBox::samplesValue(void) const
{
  return this->time * this->currSampleRate;
}

void
TimeSpinBox::setSamplesValue(qreal value)
{
  const TimeSpinBoxUnit *units = this->getCurrentSpinBoxUnit();

  this->time = value / this->currSampleRate;

  if (units->timeRelative)
    value /= this->currSampleRate;

  this->ui->valueSpin->setValue(value / units->multiplier);
}

qreal
TimeSpinBox::timeValue(void) const
{
  return this->time;
}

void
TimeSpinBox::setTimeValue(qreal value)
{
  const TimeSpinBoxUnit *units = this->getCurrentSpinBoxUnit();

  this->time = value;

  if (!units->timeRelative)
    value *= this->currSampleRate;

  this->ui->valueSpin->setValue(value / units->multiplier);
}

void
TimeSpinBox::clearUnits(void)
{
  this->units.clear();
  this->ui->unitCombo->clear();
}

void
TimeSpinBox::addUnit(QString name, bool timeRelative, qreal multiplier)
{
  this->units.push_back(TimeSpinBoxUnit(name, timeRelative, multiplier));
  this->ui->unitCombo->addItem(name);
}

QString
TimeSpinBox::getCurrentUnitName(void) const
{
  return this->getCurrentSpinBoxUnit()->name;
}

bool
TimeSpinBox::isCurrentUnitTimeRelative(void) const
{
  return this->getCurrentSpinBoxUnit()->timeRelative;
}

qreal
TimeSpinBox::getCurrentUnitMultiplier(void) const
{
  return this->getCurrentSpinBoxUnit()->multiplier;
}

void
TimeSpinBox::setSampleRate(qreal sampleRate)
{
  if (sampleRate > 0) {
    this->currSampleRate = sampleRate;
    this->adjustLimits();
  }
}

qreal
TimeSpinBox::sampleRate(void) const
{
  return this->currSampleRate;
}

void
TimeSpinBox::setBestUnits(bool timeRelative)
{
  qreal absValue = std::abs(
        timeRelative ? this->timeValue() : this->samplesValue());

  if (absValue > 0) {
    int bestUnitNdx = -1;
    qreal bestDigitCount = 0;
    qreal oldTimeValue = this->timeValue();
    for (int i = 0; i < this->units.size(); ++i) {
      if (this->units[i].timeRelative == timeRelative) {
        qreal digitCount = std::log10(absValue / this->units[i].multiplier);

        if (digitCount >= 0) {
          if (bestUnitNdx == -1 || digitCount < bestDigitCount) {
            bestUnitNdx = i;
            bestDigitCount = digitCount;
          }
        }
      }
    }

    if (bestUnitNdx != -1) {
      this->ui->unitCombo->setCurrentIndex(bestUnitNdx);
      this->adjustLimits();
      this->setTimeValue(oldTimeValue);
    }
  }
}

/////////////////////////////////// Slots //////////////////////////////////////
void
TimeSpinBox::onChangeUnits(void)
{
  qreal oldTimeValue = this->timeValue();
  this->adjustLimits();
  this->setTimeValue(oldTimeValue);
}

void
TimeSpinBox::onValueChanged(void)
{
  const TimeSpinBoxUnit *units = this->getCurrentSpinBoxUnit();
  qreal value = this->ui->valueSpin->value();
  qreal current = this->time / units->multiplier;

  if (!units->timeRelative)
    current /= this->currSampleRate;

  if (std::abs(value - current) >= 1e-2) {
    if (!units->timeRelative)
      value /= this->currSampleRate;

    this->time = value * units->multiplier;

    emit changed(this->timeValue(), this->samplesValue());
  }
}

