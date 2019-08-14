//
//    Decider.cpp: Simple symbol decider
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

#include "Decider.h"

Decider::Decider()
{

}

void
Decider::feed(const float _Complex *data, size_t len)
{
  this->buffer.resize(len);
  float arg;
  int sym;

  switch (this->mode) {
    case ARGUMENT:
      for (unsigned int i = 0; i < len; ++i) {
        SUWIDGETS_DETECT_ARGUMENT(arg, data[i]);

        sym = static_cast<int>(
              floorf((arg - this->minAngle) / (this->delta)));
        if (sym < 0)
          sym = 0;
        else if (sym >= this->intervals)
          sym = this->intervals - 1;

        this->buffer[i] = static_cast<Symbol>(sym);
      }
      break;

    case MODULUS:
      for (unsigned int i = 0; i < len; ++i) {
        SUWIDGETS_DETECT_MODULUS(arg, data[i]);

        sym = static_cast<int>(
              floorf((arg - this->minAngle) / (this->delta)));
        if (sym < 0)
          sym = 0;
        else if (sym >= this->intervals)
          sym = this->intervals - 1;

        this->buffer[i] = static_cast<Symbol>(sym);
      }
      break;
  }
}
