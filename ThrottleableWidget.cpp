//
//    ThrottleableWidget.cpp: Base class for throttleable widgets
//    Copyright (C) 2019 Gonzalo José Carracedo Carballal
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


#include "ThrottleableWidget.h"
#include <cmath>

ThrottleControl::ThrottleControl(unsigned int rate)
{
  this->rate = rate;
  this->timer.setInterval(static_cast<int>(rate));

  this->connect(
        &this->timer,
        SIGNAL(timeout()),
        this,
        SLOT(onTimerTimeout()));

  this->timer.start();
}

void
ThrottleControl::setRate(unsigned int fps)
{
  if (fps != this->rate) {
    this->rate = fps;
    this->timer.setInterval(
          static_cast<int>(1000.f / fps));
  }
}

void
ThrottleControl::setCpuBurn(bool doit)
{
  if (this->burnCpu != doit) {
    this->burnCpu = doit;
    emit cpuBurnSet(doit);
  }
}

void
ThrottleControl::onTimerTimeout(void)
{
  emit tick();
}

ThrottleableWidget::ThrottleableWidget(QWidget *parent) : QFrame(parent)
{

}

void
ThrottleableWidget::invalidate(void)
{
  this->dirty = true;

  // Throttle disabled. Trigger paintEvent
  if (!this->throttle)
    this->update();
}

void
ThrottleableWidget::invalidateHard(void)
{
  this->draw();
  this->update();
}

void
ThrottleableWidget::setThrottleControl(ThrottleControl *control)
{
  this->control = control;
  this->throttle = !control->getBurnCpu();
  this->connect(
        control,
        SIGNAL(tick()),
        this,
        SLOT(onTick()));

  this->connect(
        control,
        SIGNAL(cpuBurnSet(bool)),
        this,
        SLOT(onCpuBurnSet(bool)));

  if (!this->throttle && this->dirty)
    this->update();
}

void
ThrottleableWidget::paintEvent(QPaintEvent *)
{
  // Throttle disabled. Leverage paintEvent to draw pending stuff
  if (!this->throttle && this->dirty) {
    this->draw();
    this->dirty = false;
  }

  this->paint();
}

void
ThrottleableWidget::resizeEvent(QResizeEvent *)
{
  this->dirty = true;
  this->draw();
  this->update();
}

void
ThrottleableWidget::onTick(void)
{
  // Throttling enabled. Dump to screen
  if (this->dirty && this->throttle) {
    this->draw();
    this->dirty = false;
    this->update();
  }
}

void
ThrottleableWidget::onCpuBurnSet(bool state)
{
  // Disables throttle control. Update.
  if (state && this->dirty) {
    this->draw();
    this->dirty = false;
    this->update();
  }

  this->throttle = !state;
}

