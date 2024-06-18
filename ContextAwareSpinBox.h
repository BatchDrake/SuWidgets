//
//    ContextAwareSpinBox.h: description
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
#ifndef CONTEXTAWARESPINBOX_H
#define CONTEXTAWARESPINBOX_H

#include <QDoubleSpinBox>

class ContextAwareSpinBox : public QDoubleSpinBox
{
  Q_OBJECT

  QStyle *m_blockStyle   = nullptr;
  QStyle *m_baseStyle    = nullptr;
  bool    m_blockEnabled = false;

  using QDoubleSpinBox::QDoubleSpinBox;
  int stepToCursor(qreal) const;

public:
  ContextAwareSpinBox(QWidget *parent = nullptr);
  ~ContextAwareSpinBox() override;

  virtual void stepBy(int steps) override;
  virtual qreal currentStep() const;
  virtual void focusInEvent(QFocusEvent *) override;
  virtual void focusOutEvent(QFocusEvent *) override;

  void setSingleStep(double val);
  void setMinimumStep();
  void setBlockEnabled(bool);
  bool blockEnabled() const;

public slots:
  void onCursorPositionChanged(int, int);
};

#endif // CONTEXTAWARESPINBOX_H
