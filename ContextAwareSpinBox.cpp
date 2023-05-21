//
//    ContextAwareSpinBox.cpp: description
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

#include <QLineEdit>
#include "ContextAwareSpinBox.h"

qreal
ContextAwareSpinBox::currentStep() const
{
  int textLen = static_cast<int>(lineEdit()->text().length());
  int dec = decimals();
  int intLen = textLen - dec - 1;
  int pos = lineEdit()->cursorPosition();

  if (pos < 0)
    pos = intLen;

  // 1387.01 --> LEN = 7, DECIMALS = 2
  //             INT = 7 - 2 - 1 = 4

  // Pos 0: increment in 10000 (|1387.01)
  // Pos 1: increment in 1000  (1|387.01)
  // Pos 2: increment in 100   (13|87.01)
  // Pos 3: increment in 10    (138|7.01)
  // Pos 4, 5: increment in 1  (1387|.01)
  // Pos 6: increment in 0.1   (1387.0|1)
  // Pos 7: increment in 0.01  (1387.01|)


  // Past the floating point, we ignore its position
  if (pos > intLen)
    --pos;

  return pow(10., intLen - pos);
}

int
ContextAwareSpinBox::stepToCursor(qreal step) const
{
  int decimPos = static_cast<int>(floor(log10(step)));
  int textLen = static_cast<int>(lineEdit()->text().length());
  int dec = decimals();
  int intLen = textLen - dec - 1;

  // 1387.01 --> LEN = 7, DECIMALS = 2
  //             INT = 7 - 2 - 1 = 4

  // 10000: 0
  //  1000  1
  //   100  2
  //    10  3
  //     1  4
  //    .1  6
  //   .01  7

  int pos = intLen - decimPos;

  if (decimPos < 0)
    ++pos;

  return qBound(0, pos, textLen);
}

void
ContextAwareSpinBox::stepBy(int steps)
{
  qreal step = currentStep();

  QDoubleSpinBox::setSingleStep(step);
  QAbstractSpinBox::stepBy(steps);

  lineEdit()->setCursorPosition(stepToCursor(step));
}

void
ContextAwareSpinBox::setSingleStep(qreal step)
{
  QDoubleSpinBox::setSingleStep(step);
  lineEdit()->setCursorPosition(stepToCursor(step));
}

void
ContextAwareSpinBox::setMinimumStep()
{
  int textLen = static_cast<int>(lineEdit()->text().length());
  lineEdit()->setCursorPosition(textLen);
}
