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
#include <QKeyEvent>

FrequencySpinBox::FrequencySpinBox(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FrequencySpinBox)
{
  ui->setupUi(this);

  refreshUi();

  connectAll();

  int width = SuWidgetsHelpers::getWidgetTextWidth(
        ui->decFreqUnitsButton, "<");

  ui->incFreqUnitsButton->setMaximumWidth(4 * width);
  ui->decFreqUnitsButton->setMaximumWidth(4 * width);

  ui->frequencySpin->installEventFilter(this);
}

FrequencySpinBox::~FrequencySpinBox()
{
  delete ui;
}

QString
FrequencySpinBox::freqSuffix() const
{
  switch (m_unitMultiplier) {
    case Femto:
      return "f" + m_fUnits;

    case Pico:
      return "p" + m_fUnits;

    case Nano:
      return "n" + m_fUnits;

    case Micro:
      return "µ" + m_fUnits;

    case Milli:
      return "m" + m_fUnits;

    case None:
      return m_fUnits;

    case Kilo:
      return "k" + m_fUnits;

    case Mega:
      return "M" + m_fUnits;

    case Giga:
      return "G" + m_fUnits;

    case Tera:
      return "T" + m_fUnits;
  }

  return "??";
}

qreal
FrequencySpinBox::freqMultiplier() const
{
  switch (m_unitMultiplier) {
    case Femto:
      return 1e-15;

    case Pico:
      return 1e-12;

    case Nano:
      return 1e-9;

    case Micro:
      return 1e-6;

    case Milli:
      return 1e-3;

    case None:
      return 1;

    case Kilo:
      return 1e3;

    case Mega:
      return 1e6;

    case Giga:
      return 1e9;

    case Tera:
      return 1e12;
  }

  return 0;
}

void
FrequencySpinBox::connectAll()
{
  connect(
        ui->incFreqUnitsButton,
        SIGNAL(clicked()),
        this,
        SLOT(onIncFreqUnitMultiplier()));

  connect(
        ui->decFreqUnitsButton,
        SIGNAL(clicked()),
        this,
        SLOT(onDecFreqUnitMultiplier()));

  connect(
        ui->frequencySpin,
        SIGNAL(editingFinished()),
        this,
        SLOT(onEditingFinished()));
}


void
FrequencySpinBox::adjustUnitMultiplier()
{
  qreal absValue = std::fabs(m_currValue);

  if (absValue >= 1e12)
    setFrequencyUnitMultiplier(Tera);
  else if (absValue >= 1e9)
    setFrequencyUnitMultiplier(Giga);
  else if (absValue >= 1e6)
    setFrequencyUnitMultiplier(Mega);
  else if (absValue >= 1e3)
    setFrequencyUnitMultiplier(Kilo);
  else {
    if (m_allowSubMultiples) {
      if (absValue >= 1)
        setFrequencyUnitMultiplier(None);
      else if (absValue >= 1e-3)
        setFrequencyUnitMultiplier(Milli);
      else if (absValue >= 1e-6)
        setFrequencyUnitMultiplier(Micro);
      else if (absValue >= 1e-9)
        setFrequencyUnitMultiplier(Nano);
      else if (absValue >= 1e-12)
        setFrequencyUnitMultiplier(Pico);
      else
        setFrequencyUnitMultiplier(Femto);
    } else {
      setFrequencyUnitMultiplier(None);
    }
  }

  refreshUi();
}

bool
FrequencySpinBox::eventFilter(QObject *obj, QEvent *objEvent)
{
  bool captured = false;

  if (obj == ui->frequencySpin && objEvent->type() == QEvent::KeyPress) {
    QKeyEvent *ev = static_cast<QKeyEvent *>(objEvent);

    captured = true;

    switch (ev->key()) {
      case Qt::Key_T:
        setFrequencyUnitMultiplierOnEdit(Tera);
        break;

      case Qt::Key_G:
        setFrequencyUnitMultiplierOnEdit(Giga);
        break;

      case Qt::Key_M:
        setFrequencyUnitMultiplierOnEdit(
              m_allowSubMultiples && ev->text() == "m"
              ? Milli
              : Mega);
        break;

      case Qt::Key_K:
        setFrequencyUnitMultiplierOnEdit(Kilo);
        break;

      case Qt::Key_Space:
        setFrequencyUnitMultiplierOnEdit(None);
        break;

      case Qt::Key_U:
        setFrequencyUnitMultiplierOnEdit(Micro);
        break;

      case Qt::Key_N:
        setFrequencyUnitMultiplierOnEdit(Nano);
        break;

      case Qt::Key_P:
        setFrequencyUnitMultiplierOnEdit(Pico);
        break;

      case Qt::Key_F:
        setFrequencyUnitMultiplierOnEdit(Femto);
        break;

      default:
        captured = false;
    }
  }

  return captured;
}

void
FrequencySpinBox::refreshUiEx(bool setValue)
{
  qreal mul = 1 / freqMultiplier();

  ui->incFreqUnitsButton->setEnabled(m_unitMultiplier != Tera);
  ui->decFreqUnitsButton->setEnabled(
        m_allowSubMultiples
        ? m_unitMultiplier != Femto
        : m_unitMultiplier != None);

  BLOCKSIG_BEGIN(ui->frequencySpin);
  ui->frequencySpin->setSuffix(" " + freqSuffix());
  ui->frequencySpin->setDecimals(
        static_cast<int>(m_unitMultiplier) * 3
        + static_cast<int>(m_uExtraDecimals));

  ui->frequencySpin->setMaximum(m_max * mul);
  ui->frequencySpin->setMinimum(m_min * mul);

  if (setValue)
    ui->frequencySpin->setValue(m_currValue * mul);
  BLOCKSIG_END();
}

void
FrequencySpinBox::refreshUi()
{
  refreshUiEx(true);
}

void
FrequencySpinBox::setValue(qreal val)
{
  qreal min = m_allowSubMultiples ? m_min : 1.;

  if (fabs(val - m_currValue) >= min) {
    qreal oldValue = m_currValue;
    m_currValue = val;

    if (m_autoUnitMultiplier)
      adjustUnitMultiplier();

    refreshUi();

    if (m_currValue != oldValue)
      emit valueChanged(m_currValue);
  }
}

void
FrequencySpinBox::setSubMultiplesAllowed(bool allowed)
{
  m_allowSubMultiples = allowed;
  refreshUi();
}

bool
FrequencySpinBox::subMultiplesAllowed() const
{
  return m_allowSubMultiples;
}


qreal
FrequencySpinBox::value() const
{
  // currValue may be outdated if editing was in progress
  return ui->frequencySpin->value() * freqMultiplier();
}

void
FrequencySpinBox::setMaximum(qreal max)
{
  m_max = max;
  refreshUi();
}

qreal
FrequencySpinBox::maximum() const
{
  return m_max;
}

void
FrequencySpinBox::setMinimum(qreal min)
{
  m_min = min;
  refreshUi();
}

qreal
FrequencySpinBox::minimum() const
{
  return m_min;
}

void
FrequencySpinBox::setExtraDecimals(unsigned int extra)
{
  m_uExtraDecimals = extra;
  refreshUi();
}

unsigned int
FrequencySpinBox::extraDecimals() const
{
  return m_uExtraDecimals;
}

void
FrequencySpinBox::setAutoUnitMultiplierEnabled(bool enabled)
{
  m_autoUnitMultiplier = enabled;

  if (enabled)
    adjustUnitMultiplier();
}

bool
FrequencySpinBox::autoUnitMultiplierEnabled() const
{
  return m_autoUnitMultiplier;
}

void
FrequencySpinBox::setFrequencyUnitMultiplier(FrequencyUnitMultiplier UnitMultiplier)
{
  m_unitMultiplier = UnitMultiplier;
  refreshUi();
}

void
FrequencySpinBox::setFrequencyUnitMultiplierOnEdit(FrequencyUnitMultiplier mul)
{
  if (m_unitMultiplier != mul) {
    m_unitMultiplier = mul;
    refreshUiEx(false);
  }
}


FrequencySpinBox::FrequencyUnitMultiplier
FrequencySpinBox::frequencyUnitMultiplier() const
{
  return m_unitMultiplier;
}

void
FrequencySpinBox::incFrequencyUnitMultiplier()
{
  if (m_unitMultiplier < Tera)
    setFrequencyUnitMultiplier(
          static_cast<FrequencyUnitMultiplier>(static_cast<int>(m_unitMultiplier) + 1));

}

void
FrequencySpinBox::decFrequencyUnitMultiplier()
{
  FrequencyUnitMultiplier min = m_allowSubMultiples ? Femto : None;
  if (m_unitMultiplier > min)
    setFrequencyUnitMultiplier(
          static_cast<FrequencyUnitMultiplier>(static_cast<int>(m_unitMultiplier) - 1));
}

void
FrequencySpinBox::setUnits(QString const units)
{
  m_fUnits = units;
  refreshUi();
}

QString
FrequencySpinBox::units() const
{
  return m_fUnits;

}

void
FrequencySpinBox::setEditable(bool editable)
{
  ui->frequencySpin->setReadOnly(!editable);
}

bool
FrequencySpinBox::editable() const
{
  return !ui->frequencySpin->isReadOnly();
}

void
FrequencySpinBox::setFocus()
{
  ui->frequencySpin->setFocus();
  ui->frequencySpin->selectAll();
}

///////////////////////////////// Slots ///////////////////////////////////////
void
FrequencySpinBox::onEditingFinished()
{
  qreal prevValue = m_currValue;
  m_currValue = ui->frequencySpin->value() * freqMultiplier();

  if (fabs(prevValue - m_currValue) > 1e-15)
    emit valueChanged(m_currValue);
}

void
FrequencySpinBox::onIncFreqUnitMultiplier()
{
  incFrequencyUnitMultiplier();
}

void
FrequencySpinBox::onDecFreqUnitMultiplier()
{
  decFrequencyUnitMultiplier();
}

