//
//    QVerticalLabel.cpp: Vertically oriented label
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
#include "QVerticalLabel.h"

#include <QPainter>

QVerticalLabel::QVerticalLabel(QWidget *parent) : QLabel(parent)
{

}

QVerticalLabel::QVerticalLabel(const QString &text, QWidget *parent)
: QLabel(text, parent)
{
}

void
QVerticalLabel::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  int width, height;

  painter.translate(0, sizeHint().height());
  painter.rotate(270);

  width  = sizeHint().height();
  height = sizeHint().width();

  painter.drawText(
        QRect(
          QPoint(
            -(this->height() / 2 - width / 2),
            this->width()  / 2 - height / 2),
          QLabel::sizeHint()),
        Qt::AlignCenter | Qt::AlignVCenter,
        text());
}

QSize
QVerticalLabel::minimumSizeHint() const
{
  QSize s = QLabel::minimumSizeHint();
  return QSize(s.height(), s.width());
}

QSize
QVerticalLabel::sizeHint() const
{
  QSize s = QLabel::sizeHint();
  return QSize(s.height(), s.width());
}
