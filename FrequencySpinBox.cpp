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
#include <cmath>
#include "FrequencySpinBox.h"
#include "SuWidgetsHelpers.h"
#include "ui_FrequencySpinBox.h"

FrequencySpinBox::FrequencySpinBox(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FrequencySpinBox)
{
  ui->setupUi(this);

  this->refreshUi();

  this->connectAll();

  int width = SuWidgetsHelpers::getWidgetTextWidth(
        this->ui->decFreqUnitsButton, "<");

  this->ui->incFreqUnitsButton->setMaximumWidth(4 * width);
  this->ui->decFreqUnitsButton->setMaximumWidth(4 * width);
}

FrequencySpinBox::~FrequencySpinBox()
{
  delete ui;
}

QString
FrequencySpinBox::freqSuffix(void) const
{
  switch (this->UnitMultiplier) {
    case MUL_FEMTO:
      return "f" + this->fUnits;

    case MUL_PICO:
      return "p" + this->fUnits;

    case MUL_NANO:
      return "n" + this->fUnits;

    case MUL_MICRO:
      return "µ" + this->fUnits;

    case MUL_MILLI:
      return "m" + this->fUnits;

    case MUL_NONE:
      return this->fUnits;

    case MUL_KILO:
      return "k" + this->fUnits;

    case MUL_MEGA:
      return "M" + this->fUnits;

    case MUL_GIGA:
      return "G" + this->fUnits;

    case MUL_TERA:
      return "T" + this->fUnits;
  }

  return "??";
}

double
FrequencySpinBox::freqMultiplier(void) const
{
  switch (this->UnitMultiplier) {
    case MUL_FEMTO:
      return 1e-15;

    case MUL_PICO:
      return 1e-12;

    case MUL_NANO:
      return 1e-9;

    case MUL_MICRO:
      return 1e-6;

    case MUL_MILLI:
      return 1e-3;

    case MUL_NONE:
      return 1;

    case MUL_KILO:
      return 1e3;

    case MUL_MEGA:
      return 1e6;

    case MUL_GIGA:
      return 1e9;

    case MUL_TERA:
      return 1e12;
  }

  return 0;
}

void
FrequencySpinBox::connectAll(void)
{
  connect(
        this->ui->incFreqUnitsButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onIncFreqUnitMultiplier(void)));

  connect(
        this->ui->decFreqUnitsButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onDecFreqUnitMultiplier(void)));

  connect(
        this->ui->frequencySpin,
        SIGNAL(editingFinished(void)),
        this,
        SLOT(onEditingFinished(void)));
}


void
FrequencySpinBox::adjustUnitMultiplier(void)
{
  qreal absValue = std::fabs(this->currValue);

  if (absValue >= 1e12)
    this->setFrequencyUnitMultiplier(MUL_TERA);
  else if (absValue >= 1e9)
    this->setFrequencyUnitMultiplier(MUL_GIGA);
  else if (absValue >= 1e6)
    this->setFrequencyUnitMultiplier(MUL_MEGA);
  else if (absValue >= 1e3)
    this->setFrequencyUnitMultiplier(MUL_KILO);
  else {
    if (this->allowSubMultiples) {
      if (absValue >= 1)
        this->setFrequencyUnitMultiplier(MUL_NONE);
      else if (absValue >= 1e-3)
        this->setFrequencyUnitMultiplier(MUL_MILLI);
      else if (absValue >= 1e-6)
        this->setFrequencyUnitMultiplier(MUL_MICRO);
      else if (absValue >= 1e-9)
        this->setFrequencyUnitMultiplier(MUL_NANO);
      else if (absValue >= 1e-12)
        this->setFrequencyUnitMultiplier(MUL_PICO);
      else
        this->setFrequencyUnitMultiplier(MUL_FEMTO);
    } else {
      this->setFrequencyUnitMultiplier(MUL_NONE);
    }
  }

  this->refreshUi();
}

void
FrequencySpinBox::refreshUi(void)
{
  if (!this->refreshing) {
    double mul = 1 / this->freqMultiplier();
    this->refreshing = true;

    this->ui->incFreqUnitsButton->setEnabled(this->UnitMultiplier != MUL_TERA);
    this->ui->decFreqUnitsButton->setEnabled(
          this->allowSubMultiples
          ? this->UnitMultiplier != MUL_FEMTO
          : this->UnitMultiplier != MUL_NONE);

    this->ui->frequencySpin->setSuffix(" " + this->freqSuffix());
    this->ui->frequencySpin->setDecimals(
          static_cast<int>(this->UnitMultiplier) * 3
          + static_cast<int>(this->uExtraDecimals));

    this->ui->frequencySpin->setMaximum(this->max * mul);
    this->ui->frequencySpin->setMinimum(this->min * mul);

    this->ui->frequencySpin->setValue(this->currValue * mul);

    this->refreshing = false;
  }
}

void
FrequencySpinBox::setValue(double val)
{
  double min = this->allowSubMultiples ? this->min : 1.;

  if (fabs(val - this->currValue) >= min) {
    double oldValue = this->currValue;
    this->currValue = val;

    if (this->autoUnitMultiplier)
      this->adjustUnitMultiplier();

    this->refreshUi();

    if (this->currValue != oldValue)
      emit valueChanged(this->currValue);
  }
}

void
FrequencySpinBox::setSubMultiplesAllowed(bool allowed)
{
  this->allowSubMultiples = allowed;
  this->refreshUi();
}

bool
FrequencySpinBox::subMultiplesAllowed() const
{
  return this->allowSubMultiples;
}


double
FrequencySpinBox::value(void) const
{
  // this->currValue may be outdated if editing was in progress
  return this->ui->frequencySpin->value() * this->freqMultiplier();
}

void
FrequencySpinBox::setMaximum(double max)
{
  this->max = max;
  this->refreshUi();
}

double
FrequencySpinBox::maximum(void) const
{
  return this->max;
}

void
FrequencySpinBox::setMinimum(double min)
{
  this->min = min;
  this->refreshUi();
}

double
FrequencySpinBox::minimum(void) const
{
  return this->min;
}

void
FrequencySpinBox::setExtraDecimals(unsigned int extra)
{
  this->uExtraDecimals = extra;
  this->refreshUi();
}

unsigned int
FrequencySpinBox::extraDecimals(void) const
{
  return this->uExtraDecimals;
}

void
FrequencySpinBox::setAutoUnitMultiplierEnabled(bool enabled)
{
  this->autoUnitMultiplier = enabled;

  if (enabled)
    this->adjustUnitMultiplier();
}

bool
FrequencySpinBox::autoUnitMultiplierEnabled(void) const
{
  return this->autoUnitMultiplier;
}

void
FrequencySpinBox::setFrequencyUnitMultiplier(FrequencyUnitMultiplier UnitMultiplier)
{
  this->UnitMultiplier = UnitMultiplier;
  this->refreshUi();
}

FrequencySpinBox::FrequencyUnitMultiplier
FrequencySpinBox::frequencyUnitMultiplier(void) const
{
  return this->UnitMultiplier;
}

void
FrequencySpinBox::incFrequencyUnitMultiplier(void)
{
  if (this->UnitMultiplier < MUL_TERA)
    this->setFrequencyUnitMultiplier(
          static_cast<FrequencyUnitMultiplier>(static_cast<int>(this->UnitMultiplier) + 1));

}

void
FrequencySpinBox::decFrequencyUnitMultiplier(void)
{
  FrequencyUnitMultiplier min = this->allowSubMultiples ? MUL_FEMTO : MUL_NONE;
  if (this->UnitMultiplier > min)
    this->setFrequencyUnitMultiplier(
          static_cast<FrequencyUnitMultiplier>(static_cast<int>(this->UnitMultiplier) - 1));
}

void
FrequencySpinBox::setUnits(QString const units)
{
  this->fUnits = units;
  this->refreshUi();
}

QString
FrequencySpinBox::units(void) const
{
  return this->fUnits;

}

void
FrequencySpinBox::setEditable(bool editable)
{
  this->ui->frequencySpin->setReadOnly(!editable);
}

bool
FrequencySpinBox::editable(void) const
{
  return !this->ui->frequencySpin->isReadOnly();
}

void
FrequencySpinBox::setFocus(void)
{
  this->ui->frequencySpin->setFocus();
  this->ui->frequencySpin->selectAll();
}

///////////////////////////////// Slots ///////////////////////////////////////
void
FrequencySpinBox::onEditingFinished(void)
{
  if (!this->refreshing) {
    this->currValue = this->ui->frequencySpin->value() * this->freqMultiplier();
    emit valueChanged(this->currValue);
  }
}

void
FrequencySpinBox::onIncFreqUnitMultiplier(void)
{
  this->incFrequencyUnitMultiplier();
}

void
FrequencySpinBox::onDecFreqUnitMultiplier(void)
{
  this->decFrequencyUnitMultiplier();
}

