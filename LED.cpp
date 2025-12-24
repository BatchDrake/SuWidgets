//
//    LED.cpp: LED widget
//    Copyright (C) 2023 Gonzalo José Carracedo Carballal
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
#include "LED.h"
#include "qicon.h"

#include <QPainter>

LED::LED(QWidget *parent)
  : QWidget{parent}
{

}

void
LED::paintEvent(QPaintEvent *)
{
  if (m_haveLed) {
    QPainter painter(this);

    if (m_on)
      painter.drawPixmap(0, 0, m_ledOnPixmap);
    else
      painter.drawPixmap(0, 0, m_ledOffPixmap);
  }
}

void
LED::redraw()
{
  if (this->size().isValid()) {
    QString colStr = "red";

    switch (m_color) {
      case Red:
        colStr = "red";
        break;

      case Yellow:
        colStr = "yellow";
        break;

      case Green:
        colStr = "green";
        break;
    }

    m_geometry     = this->size();

    m_ledOnPixmap  = QIcon(":/led_on_" + colStr + ".svg").pixmap(m_geometry);
    m_ledOffPixmap = QIcon(":/led_off_" + colStr + ".svg").pixmap(m_geometry);
    m_haveLed      = true;
  }
}

void
LED::resizeEvent(QResizeEvent *)
{
  if (!this->size().isValid())
    return;

  if (m_geometry != this->size())
    redraw();
}
