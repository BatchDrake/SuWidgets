//
//    Decider.h: Simple symbol decider
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


#ifndef DECIDER_H
#define DECIDER_H

#include <QObject>
#include <cmath>
#include <complex.h>
#include <cstdint>
#include <vector>

typedef uint8_t Symbol;

#define SUWIDGETS_DETECT_ARGUMENT(dest, orig) \
  if ((dest = cargf(orig)) < 0.f) \
    dest += 2 * M_PI

#define SUWIDGETS_DETECT_MODULUS(dest, orig) \
  dest = cabsf(orig)

class Decider
{
  public:
    enum DecisionMode {
      ARGUMENT,
      MODULUS
    };

  private:
    enum DecisionMode mode = ARGUMENT;
    int bps = 1;
    int intervals = 2;
    float delta = static_cast<float>(M_PI);
    float minAngle = 0;
    float maxAngle = static_cast<float>(2 * M_PI);
    float scale = static_cast<float>(2 * M_PI);
    std::vector<Symbol> buffer;

  public:
    float
    getDelta(void) const
    {
      return this->delta;
    }

    void
    setMinAngle(float minAngle)
    {
      if (fabsf(this->minAngle - minAngle) > 1e-15f) {
        this->minAngle = minAngle;
        this->scale = this->maxAngle - this->minAngle;
        this->delta = this->scale / this->intervals;
      }
    }

    void
    setMaxAngle(float maxAngle)
    {
      if (fabsf(this->maxAngle - maxAngle) > 1e-15f) {
        this->maxAngle = maxAngle;
        this->scale = this->maxAngle - this->minAngle;
        this->delta = this->scale / this->intervals;
      }
    }

    float
    getMinAngle(void) const
    {
      return this->minAngle;
    }

    float
    getMaxAngle(void) const
    {
      return this->maxAngle;
    }

    int
    getIntervals(void) const
    {
      return this->intervals;
    }

    unsigned int
    getBps(void) const
    {
      return static_cast<unsigned int>(this->bps);
    }

    void
    setBps(unsigned int bps)
    {
      if (this->bps != static_cast<int>(bps)) {
        this->bps = static_cast<int>(bps);
        this->intervals = 1 << bps;
        this->scale = this->maxAngle - this->minAngle;
        this->delta = this->scale / this->intervals;
      }
    }

    enum DecisionMode
    getDecisionMode(void) const
    {
      return this->mode;
    }

    void
    setDecisionMode(enum DecisionMode mag)
    {
      this->mode = mag;
    }

    std::vector<Symbol> const &
    get(void) const
    {
      return this->buffer;
    }

    void feed(const float _Complex *data, size_t len);

    Decider();
};

#endif // DECIDER_H
