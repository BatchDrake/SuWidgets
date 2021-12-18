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
SuWidgetsHelpers::formatQuantity(
    qreal value,
    int precision,
    QString const &u,
    bool sign)
{
  qreal multiplier = 1;
  QString subUnits[] = { u, "m" + u, "µ" + u, "n" + u, "p" + u, "f" + u};
  QString superUnits[] = { u, "k" + u, "M" + u, "G" + u, "T" + u};
  static const qreal subUnitMultipliers[] = { 1, 1e3, 1e6, 1e9, 1e12, 1e15 };
  static const qreal superUnitMultipliers[] = { 1, 1e-3, 1e-6, 1e-9, 1e-12 };

  QString asString;
  int pfx;
  int digits, decimals;

  // Checks for degenerate cases
  if (std::isinf(value))
    return (value < 0 ? "-∞ " : "∞ ") + u;

  if (std::isnan(value))
    return "NaN " + u;

  if (std::fabs(value) < std::numeric_limits<qreal>::epsilon())
    return "0 " + u;

  if (u == "º" || u == "deg") {
    if (value < 0 && !sign)
      value += 360;
    else if (value > 180 && sign)
      value -= 360;

    if (value < 0) {
      value = -value;
      asString += "-";
    }
  } else {
    if (value < 0) {
      value = -value;
      asString += "-";
    } else if (sign) {
      asString += "+";
    }
  }

  // Get digit count.
  // 1-9:     1 digit
  // 10-99:   2 digits
  // 100-999: 3 digits
  digits = static_cast<int>(std::floor(std::log10(value))) + 1;

  if (digits > 0) {
    // Check for time units. Values above 60 are converted to hh:mm:ss
    if (u == "s") {
      qint64 seconds = static_cast<qint64>(std::floor(value));
      qreal  frac    = value - seconds;
      qint64 minutes, hours;
      int    decimalPart = 0;

      if (precision > 0) {
        multiplier = std::pow(10., precision - 1);
        decimalPart = static_cast<int>(std::round(multiplier * frac));
        if (std::fabs(decimalPart - multiplier) < 1) {
          decimalPart = 0;
          ++seconds;
        }
      }

      minutes = seconds / 60;
      hours   = minutes / 60;

      seconds %= 60;
      minutes %= 60;

      if (hours > 0) {
        asString += QString::asprintf(
              "%lld:%02lld:%02lld",
              hours,
              minutes,
              seconds);
        if (hours >= 10) {
          precision -= 6;
          decimalPart /= 100000;
        } else {
          precision -= 5;
          decimalPart /= 10000;
        }
      } else if (minutes > 0) {
        asString += QString::asprintf(
              "%lld:%02lld",
              minutes,
              seconds);
        if (minutes >= 10) {
          precision -= 4;
          decimalPart /= 1000;
        } else {
          precision -= 3;
          decimalPart /= 100;
        }
      } else {
        asString += QString::asprintf("%lld", seconds);
        precision -= digits;
      }

      if (precision > 0) {
        asString +=
            QStringLiteral(".%1").arg(
              decimalPart,
              precision,
              10,
              QLatin1Char('0'));
      }

      // No hh:mm:ss or mm::ss, add suffix to make things clear.
      if (minutes == 0 && hours == 0)
        asString += " s";
    } else if (u == "deg") {
      unsigned int deg, min, seconds;

      deg = static_cast<unsigned int>(value);
      value -= deg;

      min = static_cast<unsigned int>(value * 60);
      value -= min / 60.;

      seconds = static_cast<unsigned int>(value * 3600);
      value -= seconds / 3600.;

      asString += QString::asprintf("%02uº ", deg);
      asString += QString::asprintf("%02u' ", min);
      asString += QString::asprintf("%02u\"", seconds);

      // TODO: Add milliseconds?
    } else {
      multiplier = std::pow(10, precision - 1);
      value = std::round(value * multiplier) / multiplier;

      pfx = qBound(0, (digits - 1) / 3, u == "dB" ? 0 : 4);
      digits -= 3 * pfx;

      decimals = precision > digits ? precision - digits : 0;

      asString += QString::number(
            value * superUnitMultipliers[pfx],
            'f',
            decimals);

      asString += " " + superUnits[pfx];
    }
  } else {
    multiplier = std::pow(10, precision - digits);
    value = std::round(value * multiplier) / multiplier;
    if (value > 0)
      digits = static_cast<int>(std::floor(std::log10(value))) + 1;

    pfx = qBound(0, (3 - digits) / 3, u == "dB" ? 0 : 5);
    digits += 3 * pfx;

    decimals = precision > digits ? precision - digits : 0;

    asString += QString::number(
          value * subUnitMultipliers[pfx],
          'f',
          decimals);

    asString += " " + subUnits[pfx];
  }

  return asString;
}

QLayout *
SuWidgetsHelpers::findParentLayout(const QWidget *w)
{
  if (w->parentWidget() != nullptr)
    if (w->parentWidget()->layout() != nullptr)
      return SuWidgetsHelpers::findParentLayout(w, w->parentWidget()->layout());
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
