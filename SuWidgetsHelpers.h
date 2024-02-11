//
//    SuWidgetsHelpers.h: Common helper functions and methods
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
#ifndef SUWIDGETSHELPERS_H
#define SUWIDGETSHELPERS_H

#include <QString>
#include <QFileInfo>
#include <QRegularExpression>
#include <sigutils/types.h>
#include "Version.h"
#include <cmath>

#ifdef I
#  undef I
#endif // I

class QWidget;
class QLayout;

#define SUWIDGETS_DEFAULT_PRECISION 3
#define SCAST(type, value) static_cast<type>(value)

#define BLOCKSIG_BEGIN(object)                   \
  do {                                           \
    QObject *obj = object;                       \
    bool blocked = (object)->blockSignals(true)

#define BLOCKSIG_END()                           \
    obj->blockSignals(blocked);                  \
  } while (false)

#define BLOCKSIG(object, op)                     \
  do {                                           \
    bool blocked = (object)->blockSignals(true); \
    (object)->op;                                \
    (object)->blockSignals(blocked);             \
  } while (false)


class SuWidgetsHelpers {

    static void abiErrorAbort(unsigned int);

  public:
    struct KahanState {
      SUCOMPLEX meanSum = 0;
      SUCOMPLEX meanC = 0;

      SUFLOAT rmsSum = 0;
      SUFLOAT rmsC = 0;

      SUSCOUNT count = 0;
    };

    static QString formatPowerOf10(qreal value);
    static QString formatBinaryQuantity(qint64 value, QString units = "B");
    static int getWidgetTextWidth(const QWidget *widget, QString const &text);
    static QLayout *findParentLayout(const QWidget *);
    static QLayout *findParentLayout(const QWidget *, const QLayout *);

    static void kahanMeanAndRms(
        SUCOMPLEX *mean,
        SUFLOAT *rms,
        const SUCOMPLEX *data,
        SUSCOUNT length,
        KahanState *prevState = nullptr);

    static void calcLimits(
        SUCOMPLEX *oMin,
        SUCOMPLEX *oMax,
        const SUCOMPLEX *data,
        SUSCOUNT length,
        bool inPlace = false);

    static QString formatQuantity(
        qreal value,
        int precision,
        QString const &units = QStringLiteral("s"),
        bool sign = false);

    static inline QString
    formatQuantity(qreal value, QString const &units = QStringLiteral("s"))
    {
      int digits = 0;

      if (std::fabs(value) > 0)
        digits = static_cast<int>(std::floor(std::log10(std::fabs(value))));

      return  formatQuantity(value, digits, units);
    }

    static inline QString
    formatQuantityFromDelta(
        qreal value,
        qreal delta,
        QString const &units = QStringLiteral("s"),
        bool sign = false)
    {
      int sdigits = 0;

      if (std::fabs(value / delta) >= 1)
        sdigits = static_cast<int>(
              std::ceil(
                std::log10(
                  std::fabs(value / delta)))) + 1;

      return  formatQuantity(value, sdigits, units, sign);
    }

    static inline QString
    ensureExtension(QString const &path, QString const &ext)
    {
      QFileInfo fi(path);

      if (fi.suffix().size() == 0)
        return path + "." + ext;

      return path;
    }

    static inline QString
    extractFilterExtension(QString const &filterExpr)
    {
      QRegularExpression re(".*\\(\\*\\.([a-zA-Z0-9]*)\\)");
      QRegularExpressionMatch match = re.match(filterExpr);
      if (match.hasMatch())
        return match.captured(1);

      return "";
    }

    static inline QString
    formatComplex(SUCOMPLEX const &val)
    {
      return formatReal(SU_C_REAL(val))
      + (SU_C_IMAG(val) < 0
         ? " - " + formatReal(-SU_C_IMAG(val))
         : " + " + formatReal(SU_C_IMAG(val))) + "i";
    }

    static inline QString
    formatScientific(qreal real)
    {
      char string[32];

      snprintf(string, 32, "%+-14.6e", real);

      return QString(string);
    }

    static inline QString
    formatReal(qreal real)
    {
      char string[32];

      snprintf(string, 32, "%g", real);

      return QString(string);
    }

    static inline QString
    formatIntegerPart(qreal real)
    {
      char string[32];

      snprintf(string, 32, "%lli", static_cast<qint64>(std::floor(real)));

      return QString(string);
    }


    SuWidgetsHelpers();

    static inline void
    abiCheck(void)
    {
      if (abiVersion() != SUWIDGETS_ABI_VERSION)
        abiErrorAbort(SUWIDGETS_ABI_VERSION);
    }

    static unsigned int abiVersion(void);
    static QString version(void);
    static QString pkgversion(void);
};

#endif // SUWIDGETSHELPERS_H
