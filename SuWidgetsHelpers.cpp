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

#ifdef _WIN32
#  define localtime_r localtime_s
#endif // _WIN32

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
toSuperIndex(QString const &string)
{
  QString result = string;

  return result
        .replace(QString("0"), QString("⁰"))
        .replace(QString("1"), QString("¹"))
        .replace(QString("2"), QString("²"))
        .replace(QString("3"), QString("³"))
        .replace(QString("4"), QString("⁴"))
        .replace(QString("5"), QString("⁵"))
        .replace(QString("6"), QString("⁶"))
        .replace(QString("7"), QString("⁷"))
        .replace(QString("8"), QString("⁸"))
        .replace(QString("9"), QString("⁹"))
        .replace(QString("+"), QString("⁺"))
        .replace(QString("-"), QString("⁻"));
}

QString
SuWidgetsHelpers::formatPowerOf10(qreal value)
{
  qreal inAbs = fabs(value);
  qreal exponent = floor(log10(inAbs));
  bool  isInf = std::isinf(value);
  bool  haveExponent = std::isfinite(exponent);
  QString result = "NaN";

  if (!isInf) {
    qreal mag = 1, mantissa;
    int iExponent;
    if (haveExponent) {
      iExponent = static_cast<int>(exponent);
      if (iExponent >= 0 && iExponent < 3)
        iExponent = 0;
      haveExponent = iExponent != 0;
    } else {
      iExponent = 0;
    }

    mag = pow(10., iExponent);
    mantissa = value / mag;

    result = QString::number(mantissa);

    if (haveExponent) {
       if (result == "1")
         result = "";
       else
         result += "×";
       result += "10" + toSuperIndex(QString::number(exponent));
    }
  } else {
    result = value < 0 ? "-∞" : "∞";
  }

  return result;
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
      qint64 minutes, hours, days;
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
      days    = hours / 24;

      seconds %= 60;
      minutes %= 60;
      hours   %= 24;

      if (days > 0) {
        asString += QString::asprintf(
              "%lldd %lld:%02lld:%02lld",
              days,
              hours,
              minutes,
              seconds);
        if (days >= 10) {
          precision -= 8;
          decimalPart /= 10000000;
        } else {
          precision -= 7;
          decimalPart /= 1000000;
        }
      } else if (hours > 0) {
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
    } else if (u == "unix") {
      qint64 seconds = static_cast<qint64>(std::floor(value));
      char buf[sizeof "XXXX/XX/XX XX:XX:XX"];
      time_t unixTime;
      struct tm tm;
      qreal  frac    = value - seconds;
      int    decimalPart = 0;

      if (precision > 0) {
        multiplier = std::pow(10., precision - 1);
        decimalPart = static_cast<int>(std::round(multiplier * frac));
        if (std::fabs(decimalPart - multiplier) < 1) {
          decimalPart = 0;
          ++seconds;
        }
      }

      unixTime = static_cast<time_t>(seconds);
      localtime_r(&unixTime, &tm);
      strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", &tm);
      asString += buf;

      if (precision > 0) {
        asString +=
            QStringLiteral(".%1").arg(
              decimalPart,
              precision,
              10,
              QLatin1Char('0'));
      }
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

void
SuWidgetsHelpers::kahanMeanAndRms(
    SUCOMPLEX *mean,
    SUFLOAT *rms,
    const SUCOMPLEX *data,
    SUSCOUNT length,
    KahanState *state)
{
  KahanState currState;

  if (state == nullptr)
    state = &currState;

  SUCOMPLEX meanY, meanT;

  SUFLOAT   rmsY, rmsT;

  for (SUSCOUNT i = 0; i < length; ++i) {
    meanY = data[i] - state->meanC;
    rmsY  = SU_C_REAL(data[i] * SU_C_CONJ(data[i])) - state->rmsC;

    meanT = state->meanSum + meanY;
    rmsT  = state->rmsSum  + rmsY;

    state->meanC = (meanT - state->meanSum) - meanY;
    state->rmsC  = (rmsT  - state->rmsSum)  - rmsY;

    state->meanSum = meanT;
    state->rmsSum  = rmsT;
  }

  state->count += length;

  *mean = state->meanSum / SU_ASFLOAT(state->count);
  *rms  = SU_SQRT(state->rmsSum / state->count);
}

void
SuWidgetsHelpers::calcLimits(
    SUCOMPLEX *oMin,
    SUCOMPLEX *oMax,
    const SUCOMPLEX *data,
    SUSCOUNT length,
    bool inPlace)
{
  SUFLOAT minReal =
      inPlace ? SU_C_REAL(*oMin) : +std::numeric_limits<SUFLOAT>::infinity();
  SUFLOAT maxReal =
      inPlace ? SU_C_REAL(*oMax) : -std::numeric_limits<SUFLOAT>::infinity();


  SUFLOAT minImag =
      inPlace ? SU_C_IMAG(*oMin) : minReal;
  SUFLOAT maxImag =
      inPlace ? SU_C_IMAG(*oMax) : maxReal;

  for (SUSCOUNT i = 0; i < length; ++i) {
    if (SU_C_REAL(data[i]) < minReal)
      minReal = SU_C_REAL(data[i]);
    if (SU_C_IMAG(data[i]) < minImag)
      minImag = SU_C_IMAG(data[i]);

    if (SU_C_REAL(data[i]) > maxReal)
      maxReal = SU_C_REAL(data[i]);
    if (SU_C_IMAG(data[i]) > maxImag)
      maxImag = SU_C_IMAG(data[i]);
  }

  *oMin = minReal + SU_I * minImag;
  *oMax = maxReal + SU_I * maxImag;
}
