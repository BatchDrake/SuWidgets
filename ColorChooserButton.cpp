//
//    ColorChooserButton.cpp: Color chooser button widget
//    Copyright (C) 2025 Gonzalo José Carracedo Carballal
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

#include <QPainter>
#include <QColorDialog>

#include "ColorChooserButton.h"
#include "ui_ColorChooserButton.h"

ColorChooserButton::ColorChooserButton(QWidget *parent) :
  QWidget(parent),
  current(0, 0, 0),
  preview(
    COLOR_CHOOSER_BUTTON_PREVIEW_WIDTH,
    COLOR_CHOOSER_BUTTON_PREVIEW_HEIGHT),
  ui(new Ui::ColorChooserButton)
{
  ui->setupUi(this);
  this->ui->pushButton->setIconSize(
        QSize(
          COLOR_CHOOSER_BUTTON_PREVIEW_WIDTH,
          COLOR_CHOOSER_BUTTON_PREVIEW_HEIGHT));
  this->connect(
        this->ui->pushButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onClicked(void)));

  this->resetPixmap();
}

void
ColorChooserButton::resetPixmap(void)
{
  QPainter painter(&this->preview);

  painter.fillRect(
        0,
        0,
        COLOR_CHOOSER_BUTTON_PREVIEW_WIDTH,
        COLOR_CHOOSER_BUTTON_PREVIEW_HEIGHT,
        this->current);

  this->ui->pushButton->setIcon(QIcon(this->preview));
}

void
ColorChooserButton::onClicked(void)
{
  QColor color = QColorDialog::getColor(this->current, this);
  if (color.isValid())
    this->setColor(color);
}

ColorChooserButton::~ColorChooserButton()
{
  delete ui;
}
