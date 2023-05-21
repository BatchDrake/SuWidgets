//
//    SciSpinBox.cpp: description
//    Copyright (C) 2023 Gonzalo José Carracedo Carballal
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
#include "SciSpinBox.h"
#include "SuWidgetsHelpers.h"
#include "ui_SciSpinBox.h"
#include <cmath>
#include <climits>

SciSpinBox::SciSpinBox(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::SciSpinBox)
{
  ui->setupUi(this);

  updateRepresentation();
  connectAll();

  ui->sciButton->installEventFilter(this);
  ui->decButton->installEventFilter(this);
  ui->sciInputEdit->installEventFilter(this);
  ui->decSpinButton->installEventFilter(this);


  ui->stackedWidget->setCurrentIndex(0);

}

SciSpinBox::~SciSpinBox()
{
  delete ui;
}

bool
SciSpinBox::lostAllFocus() const
{
  bool childHasFocus;

  if (ui->stackedWidget->currentIndex() == 1) {
    childHasFocus =
           ui->decButton->hasFocus()
        || ui->sciInputEdit->hasFocus()
        || ui->sciEditPage->hasFocus();
  } else if (ui->stackedWidget->currentIndex() == 2) {
    childHasFocus =
           ui->sciButton->hasFocus()
        || ui->decSpinButton->hasFocus()
        || ui->decEditPage->hasFocus()
        || ui->decExponentLabel->hasFocus()
        || ui->decUnitsLabel->hasFocus()
        || ui->decMultiplierLabel->hasFocus();
  } else {
    childHasFocus = true;
  }

  return !childHasFocus;
}

bool
SciSpinBox::isEditing() const
{
  return ui->stackedWidget->currentWidget() == 0;
}

void
SciSpinBox::enterEditMode()
{
  if (m_decPreferred)
    changeToDec();
  else
    changeToSci();
}

void
SciSpinBox::leaveEditMode()
{
  ui->stackedWidget->setCurrentIndex(0);
}

bool
SciSpinBox::eventFilter(QObject *object, QEvent *event)
{
  switch (event->type()) {
    case QEvent::FocusOut:
      if (lostAllFocus())
        leaveEditMode();

      break;

    default:
      break;
  }

  return QWidget::eventFilter(object, event);
}

void
SciSpinBox::focusInEvent(QFocusEvent *event)
{
  if (!isEditing())
    enterEditMode();

  QWidget::focusInEvent(event);
}

void
SciSpinBox::focusOutEvent(QFocusEvent *event)
{
  if (isEditing())
    leaveEditMode();

  QWidget::focusOutEvent(event);
}


void
SciSpinBox::mousePressEvent(QMouseEvent *event)
{
  if (!isEditing())
    enterEditMode();

  QWidget::mousePressEvent(event);
}

void
SciSpinBox::connectAll()
{
  connect(
        ui->decButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onDecClicked()));

  connect(
        ui->sciButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSciClicked()));

  connect(
        ui->decSpinButton,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onDecValueChanged()));

  connect(
        ui->sciInputEdit,
        SIGNAL(editingFinished()),
        this,
        SLOT(onSciEdited()));
}

unsigned int
SciSpinBox::significant() const
{
  qreal absMin = fabs(m_min);
  qreal range  = m_max - m_min;
  int minDigits;

  // This is not so easy:
  //
  // Let us call range: R = (max - min)
  // Therefore, any number is something like N = min + tR, with t between 0 and 1
  //
  // Let us say min is 1000, and R = 1e-3. This happens if the number goes from
  // 1000 to 1000.001. If we truncate to 3, we see 1000 for all t. On the
  // other hand, if we truncate to 6, we see everyhing. Therefore, there is
  // a minimum effective decimal representation: 6 significant numbers.

  // Degenerate case.
  if (range < std::numeric_limits<qreal>::epsilon())
    return decimals();

  // We are near zero.
  if (absMin < 10)
    return decimals();

  minDigits = static_cast<int>(floor(log10(absMin))) - static_cast<int>(floor(log10(range)));

  if (minDigits > static_cast<int>(decimals()))
    return static_cast<unsigned int>(minDigits);

  return decimals();
}

void
SciSpinBox::updateRepresentation()
{
  qreal inAbs = fabs(m_value);
  qreal exponent = floor(log10(inAbs));
  bool  isNumber = std::isfinite(m_value);
  bool  haveExponent = std::isfinite(exponent);

  if (isNumber) {
    qreal mag = 1;
    if (haveExponent) {
      m_exponent = static_cast<int>(exponent);
      if (m_exponent >= 0 && m_exponent < 3)
        m_exponent = 0;
      haveExponent = m_exponent != 0;
    } else {
      m_exponent = 0;
    }

    mag = pow(10., m_exponent);
    m_mantissa = m_value / mag;

    m_haveExponent = haveExponent;

    // Draw units
    ui->unitsLabel->setText(m_units);
    ui->decUnitsLabel->setText(m_units);
    ui->sciUnitsLabel->setText(m_units);

    // Draw exponent
    ui->exponentLabel->setVisible(m_haveExponent);
    ui->multiplierLabel->setVisible(m_haveExponent);
    ui->decExponentLabel->setVisible(m_haveExponent);
    ui->decMultiplierLabel->setVisible(m_haveExponent);

    ui->sciInputEdit->setStyleSheet("background-color: palette(base);");

    if (m_haveExponent) {
      ui->exponentLabel->setText(QString::number(m_exponent));
      ui->decExponentLabel->setText(QString::number(m_exponent));
    }

    // Draw mantissa
    unsigned int currSig = significant();
    unsigned int fullRep = currSig + 2;
    QString qFormat = QString::asprintf(
          "%d.%d",
          fullRep,
          currSig);
    std::string format = qFormat.toStdString();
    format = (forceSign() ? "%+" : "%") + format + "lf";
    const char *fmt = format.c_str();

    // This warning about the format string is WRONG
    ui->mantissaLabel->setText(QString::asprintf(fmt, m_mantissa));

    // We write the number directly in scientific notation
    BLOCKSIG_BEGIN(ui->sciInputEdit);
      format = qFormat.toStdString();
      format = (forceSign() ? "%+" : "%") + format + "le";
      fmt = format.c_str();
      ui->sciInputEdit->setText(QString::asprintf(fmt, m_value));
    BLOCKSIG_END();

    // Adjust the spibox to something meaningful
    BLOCKSIG_BEGIN(ui->decSpinButton);
      ui->decSpinButton->setMinimum(m_min / mag);
      ui->decSpinButton->setMaximum(m_max / mag);
      ui->decSpinButton->setDecimals(SCAST(int, currSig));
      ui->decSpinButton->setSingleStep(1e-1);
      ui->decSpinButton->setValue(m_mantissa);
    BLOCKSIG_END();
  }
}

qreal
SciSpinBox::value() const
{
  return m_value;
}

void
SciSpinBox::setValue(qreal value)
{
  value = qBound(m_min, value, m_max);

  if (fabs(value - m_value) > std::numeric_limits<qreal>::epsilon()) {
    m_value = value;
    updateRepresentation();
    emit valueChanged(m_value);
  }
}

qreal
SciSpinBox::minimum() const
{
  return m_min;
}

void
SciSpinBox::setMinimum(qreal min)
{
  if (min > m_max)
    min = m_max;

  if (fabs(min - m_min) > std::numeric_limits<qreal>::epsilon()) {
    m_min = min;

    emit limitsChanged();
    if (m_value < m_min)
      setValue(m_min);
  }
}

qreal
SciSpinBox::maximum() const
{
  return m_max;
}

void
SciSpinBox::setMaximum(qreal max)
{
  if (max < m_min)
    max = m_min;

  if (fabs(max - m_max) > std::numeric_limits<qreal>::epsilon()) {
    m_max = max;

    emit limitsChanged();
    if (m_value > m_max)
      setValue(m_max);
  }
}

bool
SciSpinBox::forceSign() const
{
  return m_signed;
}

void
SciSpinBox::setForceSign(bool force)
{
  if (m_signed != force) {
    m_signed = force;
    updateRepresentation();

    emit signChanged();
  }
}

QString
SciSpinBox::units() const
{
  return m_units;
}

void
SciSpinBox::setUnits(QString const &units)
{
  if (units != m_units) {
    m_units = units;
    updateRepresentation();

    emit unitsChanged();
  }
}

unsigned int
SciSpinBox::decimals() const
{
  return m_decimals;
}

void
SciSpinBox::setDecimals(unsigned int decimals)
{
  if (m_decimals != decimals) {
    m_decimals = decimals;
    updateRepresentation();

    emit decimalsChanged();
  }
}

void
SciSpinBox::changeToDec()
{
  m_decPreferred = true;
  ui->stackedWidget->setCurrentIndex(2);
  ui->decSpinButton->selectAll();
  ui->decSpinButton->setFocus();
}

void
SciSpinBox::changeToSci()
{
  m_decPreferred = false;
  ui->stackedWidget->setCurrentIndex(1);
  ui->sciInputEdit->selectAll();
  ui->sciInputEdit->setFocus();
}

///////////////////////////////// Slots ///////////////////////////////////////
void
SciSpinBox::onDecClicked()
{
  changeToDec();
}

void
SciSpinBox::onSciClicked()
{
  changeToSci();
}

void
SciSpinBox::onSciEdited()
{
  qreal value;
  bool ok = false;

  if (ui->sciInputEdit->text().size() == 0) {
    updateRepresentation();
  } else {
    value = ui->sciInputEdit->text().toDouble(&ok);

    if (!ok) {
      ui->sciInputEdit->setStyleSheet("background-color: #ff7f7f;");
    } else {
      ui->sciInputEdit->setStyleSheet("background-color: palette(base);");
      setValue(value);
    }
  }
}

void
SciSpinBox::onDecValueChanged()
{
  setValue(ui->decSpinButton->value() * pow(10., m_exponent));
}
