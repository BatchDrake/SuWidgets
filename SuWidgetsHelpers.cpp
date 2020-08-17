//
//    SuWidgetsHelpers.cpp: Common helper functions
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

#include "SuWidgetsHelpers.h"

#include <QWidget>
#include <QFont>
#include <iostream>
#include <QLayout>

#ifndef SUWIDGETS_PKGVERSION
#  define SUWIDGETS_PKGVERSION \
  "custom build on " __DATE__ " at " __TIME__ " (" __VERSION__ ")"
#endif /* SUSCAN_BUILD_STRING */

SuWidgetsHelpers::SuWidgetsHelpers()
{

}

QString
SuWidgetsHelpers::version(void)
{
  return QString(SUWIDGETS_VERSION_STRING);
}

QString
SuWidgetsHelpers::pkgversion(void)
{
  return QString(SUWIDGETS_PKGVERSION);
}

unsigned int
SuWidgetsHelpers::abiVersion(void)
{
  return SUWIDGETS_ABI_VERSION;
}

void
SuWidgetsHelpers::abiErrorAbort(unsigned int user)
{
  std::cerr
      << "SuWidgets ABI mismatch. Headers are v"
      << user
      << " but library is v"
      << SUWIDGETS_ABI_VERSION
      << std::endl;
}

int
SuWidgetsHelpers::getWidgetTextWidth(const QWidget *widget, QString const &text)
{
  QFontMetrics metrics(widget->font());

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  return metrics.horizontalAdvance(text);
#else
  return metrics.width(text);
#endif // QT_VERSION_CHECK
}

QString
SuWidgetsHelpers::formatBinaryQuantity(qint64 quantity, QString units)
{
  qint64 absQuantity = std::abs(quantity);

  if (absQuantity < (1 << 10))
    return QString::number(quantity) + " " + (units == "B" ? "bytes" : units);
  else if (absQuantity < (1 << 20))
    return QString::number(static_cast<qreal>(quantity) / (1 << 10), 'f', 3) + " Ki" + units;
  else if (absQuantity < (1 << 30))
    return QString::number(static_cast<qreal>(quantity) / (1 << 20), 'f', 3) + " Mi" + units;

  return QString::number(static_cast<qreal>(quantity) / (1 << 30), 'f', 3) + " Gi" + units;
}

QString
SuWidgetsHelpers::formatQuantity(qreal value, int digits, QString units)
{
  qreal multiplier = 1;
  QString subUnits[] = {
    units,
    "m" + units,
    "μ" + units,
    "n" + units,
    "p" + units,
    "f" + units};
  QString superUnits[] = {
    units,
    "k" + units,
    "M" + units,
    "G" + units,
    "T" + units};
  QString num;
  int i = 0;

  if (std::isinf(value))
    return (value < 0 ? "-∞ " : "∞ ") + units;

  if (std::isnan(value))
    return "NaN " + units;

  if (units == "dB") {
    if (digits < 0)
      digits = -digits;
    else
      digits = 0;

    return QString::number(value, 'f', digits) + " dB";
  }

  if (digits >= 0) {
    if (digits > 1 && units == "s") { // This is too long. Format to minutes and seconds
      char time[64];
      int seconds = static_cast<int>(value);
      QString sign;
      if (seconds < 0) {
        seconds = 0;
        sign = "-";
      }
      int minutes = seconds / 60;
      int hours   = seconds / 3600;

      seconds %= 60;
      minutes %= 60;

      if (hours > 0)
        snprintf(time, sizeof(time), "%02d:%02d:%02d", hours, minutes, seconds);
      else
        snprintf(time, sizeof(time), "%02d:%02d", minutes, seconds);
      num = sign + QString(time);
    } else {
      unsigned int pfx = 0;

      while (digits > 3 && pfx < 4) {
        multiplier *= 1e-3;
        digits -= 3;
        ++pfx;
      }

      num.setNum(value * multiplier, 'f', digits);
      num += " " + superUnits[pfx];
    }
  } else {
    while (i++ < 6 && digits < -1) {
      multiplier *= 1e3;
      digits += 3;
    }

    if (digits > 0)
      digits = 0;

    num.setNum(value * multiplier, 'f', -digits);
    num += " " + subUnits[i - 1];
    if (units != "s" && value > 0)
      num = "+" + num;
  }

  return num;
}

QLayout *
SuWidgetsHelpers::findParentLayout(const QWidget *w)
{
  if (w->parentWidget() != nullptr)
    if (w->parentWidget()->layout() != nullptr)
      return findParentLayout(w, w->parentWidget()->layout());
  return nullptr;
}

QLayout *
SuWidgetsHelpers::findParentLayout(
    const QWidget *w,
    const QLayout *topLevelLayout)
{
  for (QObject* qo: topLevelLayout->children()) {
    QLayout *layout = qobject_cast<QLayout *>(qo);
    if (layout != nullptr) {
      if (layout->indexOf(const_cast<QWidget *>(w)) > -1)
        return layout;
      else if (!layout->children().isEmpty()) {
        layout = findParentLayout(w, layout);
        if (layout != nullptr)
          return layout;
      }
    }
  }

  return nullptr;
}
