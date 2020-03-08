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
#include <cmath>

class SuWidgetsHelpers {

  public:
    static QString formatBinaryQuantity(qint64 value, QString units = "B");

    static QString formatQuantity(
        qreal value,
        int digits,
        QString units = "s");

    static inline QString
    formatQuantity(qreal value, QString units = "s")
    {
      int digits = 0;

      if (std::fabs(value) > 0)
        digits = static_cast<int>(std::floor(std::log10(std::fabs(value))));

      return SuWidgetsHelpers::formatQuantity(value, digits, units);
    }

    static inline QString
    formatQuantityNearest(
        qreal value,
        int decimals,
        QString units = "s")
    {
      int digits = 0;

      if (std::fabs(value) > std::numeric_limits<qreal>::epsilon())
        digits = 3 * std::floor(std::log10(value) / 3) + decimals;

      return SuWidgetsHelpers::formatQuantity(
            value,
            digits,
            units);
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

      snprintf(string, 32, "%+-30.6e", real);

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
};

#endif // SUWIDGETSHELPERS_H
