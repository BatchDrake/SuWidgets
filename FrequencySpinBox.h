//
//    FrequencySpinBox.h: Frequency spinbox widget
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
#ifndef FREQUENCYSPINBOX_H
#define FREQUENCYSPINBOX_H

#include <QWidget>

namespace Ui {
  class FrequencySpinBox;
}

class FrequencySpinBox : public QWidget
{
  Q_OBJECT


public:
  enum FrequencyUnitMultiplier {
    Femto = -5,
    Pico  = -4,
    Nano  = -3,
    Micro = -2,
    Milli = -1,
    None  =  0,
    Kilo,
    Mega,
    Giga,
    Tera
  };

private:
  FrequencyUnitMultiplier m_unitMultiplier = None;
  QString                 m_fUnits = "Hz";
  bool                    m_autoUnitMultiplier = true;
  qreal                   m_currValue = 0;
  qreal                   m_max = 18e9;
  qreal                   m_min = 0;
  bool                    m_expectingFirstClick = false;
  unsigned int            m_uExtraDecimals = 0;

  bool                    m_allowSubMultiples = false;

  void connectAll();

  void refreshUi();
  qreal freqMultiplier() const;
  QString freqSuffix() const;

  void refreshUiEx(bool setValue);
  void setFrequencyUnitMultiplierOnEdit(FrequencyUnitMultiplier);

public:
  explicit FrequencySpinBox(QWidget *parent = nullptr);
  ~FrequencySpinBox();

  virtual bool eventFilter(QObject *obj, QEvent *event) override;
  void adjustUnitMultiplier();

  void setValue(qreal);
  qreal value() const;

  void setMaximum(qreal);
  qreal maximum() const;

  void setMinimum(qreal);
  qreal minimum() const;

  void setExtraDecimals(unsigned int);
  unsigned int extraDecimals() const;

  void setSubMultiplesAllowed(bool);
  bool subMultiplesAllowed() const;

  void setAutoUnitMultiplierEnabled(bool);
  bool autoUnitMultiplierEnabled() const;

  void setFrequencyUnitMultiplier(FrequencyUnitMultiplier UnitMultiplier);
  FrequencyUnitMultiplier frequencyUnitMultiplier() const;

  void setUnits(QString const units);
  QString units() const;

  void setEditable(bool);
  bool editable() const;

  void incFrequencyUnitMultiplier();
  void decFrequencyUnitMultiplier();

  void setFocus();

signals:
  void valueChanged(qreal freq);

public slots:
  void onEditingFinished();
  void onIncFreqUnitMultiplier();
  void onDecFreqUnitMultiplier();

private:
  Ui::FrequencySpinBox *ui;
};

#endif // FREQUENCYSPINBOX_H
