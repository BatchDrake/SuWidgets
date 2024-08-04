//
//    LED.h: description
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
#ifndef LED_H
#define LED_H

#include <QWidget>

class LED : public QWidget
{
  Q_OBJECT

  Q_PROPERTY(
      LEDColor
      color
      READ color
      WRITE setColor
      NOTIFY colorChanged)

  Q_PROPERTY(
      bool
      on
      READ isOn
      WRITE setOn
      NOTIFY onChanged)

  QPixmap m_ledOnPixmap;
  QPixmap m_ledOffPixmap;
  QSize   m_geometry;
  bool    m_haveLed = false;

public:
  enum LEDColor {
    Red,
    Yellow,
    Green
  };

  Q_ENUM(LEDColor)

  void
  setColor(LEDColor color) {
    if (color != m_color) {
      m_color = color;
      redraw();
      update();
      emit colorChanged(color);
    }
  }

  LEDColor color() const { return m_color; }

  void
  setOn(bool on) {
    if (m_on != on) {
      m_on = on;
      update();
      emit onChanged(on);
    }
  }
  bool isOn() const { return m_on; }

  explicit LED(QWidget *parent = nullptr);

  void redraw();

  void resizeEvent(QResizeEvent *) override;
  void paintEvent(QPaintEvent *) override;

signals:
  void colorChanged(LEDColor);
  void onChanged(bool);

private:
  LEDColor m_color = Red;
  bool     m_on    = false;
};

#endif // LED_H
