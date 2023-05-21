//
//    SciSpinBox.h: description
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
#ifndef SCISPINBOX_H
#define SCISPINBOX_H

#include <QWidget>

namespace Ui {
  class SciSpinBox;
}

class SciSpinBox : public QWidget
{
  Q_OBJECT

  Q_PROPERTY(
      qreal
      value
      READ value
      WRITE setValue
      NOTIFY valueChanged)

  Q_PROPERTY(
      qreal
      minimum
      READ minimum
      WRITE setMinimum
      NOTIFY limitsChanged)

  Q_PROPERTY(
      qreal
      maximum
      READ maximum
      WRITE setMaximum
      NOTIFY limitsChanged)

  Q_PROPERTY(
      bool
      forceSign
      READ forceSign
      WRITE setForceSign
      NOTIFY signChanged)

  Q_PROPERTY(
      QString
      units
      READ units
      WRITE setUnits
      NOTIFY unitsChanged)

  Q_PROPERTY(
      unsigned int
      decimals
      READ decimals
      WRITE setDecimals
      NOTIFY decimalsChanged)

  qreal m_value = .5;
  qreal m_min   = -1;
  qreal m_max   = +1;

  bool  m_signed = false;
  QString m_units = "";
  unsigned int m_decimals = 3;

  // UI state
  bool m_decPreferred = false;

  // Support methods
  qreal m_mantissa = 5;
  int   m_exponent = -1;
  bool  m_haveExponent = true;

  void updateRepresentation();
  void connectAll();

  void changeToDec();
  void changeToSci();

  bool lostAllFocus() const;

  bool isEditing() const;
  void enterEditMode();
  void leaveEditMode();

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void focusInEvent(QFocusEvent *event) override;
  void focusOutEvent(QFocusEvent *event) override;
  bool eventFilter(QObject *object, QEvent *event) override;

public:
  explicit SciSpinBox(QWidget *parent = nullptr);
  virtual ~SciSpinBox();

  qreal value() const;
  void setValue(qreal);

  qreal minimum() const;
  void setMinimum(qreal);

  qreal maximum() const;
  void setMaximum(qreal);

  bool forceSign() const;
  void setForceSign(bool);

  QString units() const;
  void setUnits(QString const &);

  unsigned int decimals() const;
  void setDecimals(unsigned int);

  unsigned int significant() const;

signals:
  void valueChanged(qreal);
  void limitsChanged();
  void signChanged();
  void unitsChanged();
  void decimalsChanged();

public slots:
  void onDecClicked();
  void onSciClicked();
  void onSciEdited();
  void onDecValueChanged();

private:
  Ui::SciSpinBox *ui;
};

#endif // SCISPINBOX_H
