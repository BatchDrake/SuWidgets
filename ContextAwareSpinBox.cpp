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
#include <cmath>
#include "ContextAwareSpinBox.h"
#include <QProxyStyle>

class BlockCursorStyle : public QProxyStyle
{
  ContextAwareSpinBox *m_spinBox = nullptr;

public:
  BlockCursorStyle(QStyle *style, ContextAwareSpinBox *spinbox) : QProxyStyle(style)
  {
    m_spinBox = spinbox;
  }

  int pixelMetric(
      PixelMetric,
      const QStyleOption *option = nullptr,
      const QWidget *widget = nullptr) const override;
};

int BlockCursorStyle::pixelMetric(
    PixelMetric metric,
    const QStyleOption *option,
    const QWidget *widget) const
{
  switch (metric) {
    case PM_TextCursorWidth:
      return m_spinBox->blockEnabled()
          ? -9
          : QProxyStyle::pixelMetric(metric, option, widget);

    default:
      return QProxyStyle::pixelMetric(metric, option, widget);
  }
}

void
ContextAwareSpinBox::focusInEvent(QFocusEvent *event)
{
  int prefixLen = static_cast<int>(prefix().size());
  int suffixLen = static_cast<int>(suffix().size());
  int textLen = static_cast<int>(lineEdit()->text().length());
  int dec = decimals();
  int decSize = dec > 0 ? dec + 1 : 0;
  int intLen = textLen - decSize - (prefixLen + suffixLen);

  lineEdit()->setCursorPosition(prefixLen + intLen);

  QDoubleSpinBox::focusInEvent(event);
}

void
ContextAwareSpinBox::focusOutEvent(QFocusEvent *event)
{
  QDoubleSpinBox::focusOutEvent(event);
}

ContextAwareSpinBox::ContextAwareSpinBox(QWidget *parent) : QDoubleSpinBox(parent)
{
  m_baseStyle  = lineEdit()->style();
  m_blockStyle = new BlockCursorStyle(m_baseStyle, this);

  connect(
        lineEdit(),
        SIGNAL(cursorPositionChanged(int, int)),
        this,
        SLOT(onCursorPositionChanged(int, int)));

  lineEdit()->setStyle(m_blockStyle);
}

ContextAwareSpinBox::~ContextAwareSpinBox()
{
}

qreal
ContextAwareSpinBox::currentStep() const
{
  int prefixLen = static_cast<int>(prefix().size());
  int suffixLen = static_cast<int>(suffix().size());
  int textLen = static_cast<int>(lineEdit()->text().length());
  int dec = decimals();
  int decSize = dec > 0 ? dec + 1 : 0;
  int intLen = textLen - decSize - (prefixLen + suffixLen);
  int pos = lineEdit()->cursorPosition() - prefixLen;

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
  int prefixLen = static_cast<int>(prefix().size());
  int suffixLen = static_cast<int>(suffix().size());
  int textLen = static_cast<int>(lineEdit()->text().length());
  int dec = decimals();
  int decSize = dec > 0 ? dec + 1 : 0;
  int intLen = textLen - decSize - (prefixLen + suffixLen);

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

  return qBound(0, pos, textLen) + prefixLen;
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

void
ContextAwareSpinBox::onCursorPositionChanged(int oldPos, int newPos)
{
  int prefixLen = static_cast<int>(prefix().size());
  int suffixLen = static_cast<int>(suffix().size());
  int textLen = static_cast<int>(lineEdit()->text().length());
  int dec = decimals();
  int decSize = dec > 0 ? dec + 1 : 0;
  int intLen = textLen - decSize - (prefixLen + suffixLen);
  int pos = lineEdit()->cursorPosition() - prefixLen;

  if (pos == intLen + 1) {
    if (oldPos < newPos) // Moving forward
      lineEdit()->setCursorPosition(prefixLen + pos + 1);
    else // Whatever is this
      lineEdit()->setCursorPosition(prefixLen + pos - 1);
    return;
  }

  if (pos > intLen + decSize)
    lineEdit()->setCursorPosition(prefixLen + intLen + decSize);
  else if (pos < 0)
    lineEdit()->setCursorPosition(prefixLen);
}

void
ContextAwareSpinBox::setBlockEnabled(bool en)
{
  if (en != m_blockEnabled) {
    m_blockEnabled = en;
    update();
  }
}

bool
ContextAwareSpinBox::blockEnabled() const
{
  return m_blockEnabled;
}
