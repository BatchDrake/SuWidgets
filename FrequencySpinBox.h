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
    MUL_NONE,
    MUL_KILO,
    MUL_MEGA,
    MUL_GIGA,
    MUL_TERA
  };

private:
  FrequencyUnitMultiplier UnitMultiplier = MUL_NONE;
  QString fUnits = "Hz";
  bool autoUnitMultiplier = true;
  double currValue = 0;
  double max = 18e9;
  double min = 0;
  bool changed = false;
  bool expectingFirstClick = false;
  bool refreshing = false;
  unsigned int uExtraDecimals = 0;

  void connectAll(void);
  void refreshUi(void);
  double freqMultiplier(void) const;
  QString freqSuffix(void) const;

public:
  explicit FrequencySpinBox(QWidget *parent = nullptr);
  ~FrequencySpinBox();

  void adjustUnitMultiplier(void);

  void setValue(double);
  double value(void) const;

  void setMaximum(double);
  double maximum(void) const;

  void setMinimum(double);
  double minimum(void) const;

  void setExtraDecimals(unsigned int);
  unsigned int extraDecimals(void) const;

  void setAutoUnitMultiplierEnabled(bool);
  bool autoUnitMultiplierEnabled(void) const;

  void setFrequencyUnitMultiplier(FrequencyUnitMultiplier UnitMultiplier);
  FrequencyUnitMultiplier frequencyUnitMultiplier(void) const;

  void setUnits(QString const units);
  QString units(void) const;

  void setEditable(bool);
  bool editable(void) const;

  void incFrequencyUnitMultiplier(void);
  void decFrequencyUnitMultiplier(void);

  bool eventFilter(QObject *, QEvent *) override;

signals:
  void valueChanged(double freq);

public slots:
  void onValueChanged(double freq);
  void onIncFreqUnitMultiplier(void);
  void onDecFreqUnitMultiplier(void);
  void onEditingFinished(void);

private:
  Ui::FrequencySpinBox *ui;
};

#endif // FREQUENCYSPINBOX_H
