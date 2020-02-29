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

    SuWidgetsHelpers();
};

#endif // SUWIDGETSHELPERS_H
