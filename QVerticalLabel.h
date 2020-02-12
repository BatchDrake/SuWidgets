//
//    QVerticalLabel.h: Vertically oriented label
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
#ifndef QVerticalLabel_H
#define QVerticalLabel_H

#include <QLabel>

class QVerticalLabel : public QLabel
{
    Q_OBJECT

public:
    explicit QVerticalLabel(QWidget *parent = nullptr);
    explicit QVerticalLabel(const QString &text, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent*);
    QSize sizeHint() const ;
    QSize minimumSizeHint() const;
};

#endif // QVerticalLabel_H
