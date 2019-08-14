//
//    ThrottleableWidget.h: Base class for throttleable widgets
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


#ifndef THROTTLEABLEWIDGET_H
#define THROTTLEABLEWIDGET_H

#include <QFrame>
#include <QTimer>

#define THROTTLE_CONTROL_DEFAULT_RATE 60

class ThrottleControl : public QObject
{
    Q_OBJECT

    QTimer timer;
    unsigned int rate = THROTTLE_CONTROL_DEFAULT_RATE;

    bool burnCpu = false;
    bool pad[3];

  public:
    explicit ThrottleControl(unsigned int rate = THROTTLE_CONTROL_DEFAULT_RATE);

    void setRate(unsigned int fps);
    void setCpuBurn(bool doit);

    bool
    getBurnCpu(void) const
    {
      return this->burnCpu;
    }

    void
    getBurnCpu(bool &ref) const
    {
      ref = this->burnCpu;
    }

    unsigned int
    getRate(void) const
    {
      return this->rate;
    }

    void
    getRate(unsigned int &rate)
    {
      rate = this->rate;
    }

  signals:
    void tick(void);
    void cpuBurnSet(bool doit);

  public slots:
    void onTimerTimeout(void);

};

class ThrottleableWidget : public QFrame
{
    Q_OBJECT

    ThrottleControl *control = nullptr; // Weak
    bool throttle = false;
    bool dirty = false;
    bool pad[6];

  public:
    explicit ThrottleableWidget(QWidget *parent = nullptr);
    void invalidate(void);
    void invalidateHard(void);
    void setThrottleControl(ThrottleControl *control);
    virtual void draw(void) = 0;
    virtual void paint(void) = 0;

    void paintEvent(QPaintEvent *ev);
    void resizeEvent(QResizeEvent *ev);

  signals:

  public slots:
    void onCpuBurnSet(bool);
    void onTick(void);
};

#endif // THROTTLEABLEWIDGET_H
