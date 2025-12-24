//
//    ColorChooserButton.h: Color chooser button widget
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

#ifndef COLORCHOOSERBUTTON_H
#define COLORCHOOSERBUTTON_H

#include <QWidget>
#include <QColor>

#define COLOR_CHOOSER_BUTTON_PREVIEW_WIDTH  48
#define COLOR_CHOOSER_BUTTON_PREVIEW_HEIGHT 16

namespace Ui {
  class ColorChooserButton;
}

class ColorChooserButton : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(
        QColor color
        READ getColor
        WRITE setColor
        NOTIFY colorChanged)

    QColor current;
    QPixmap preview;

    void resetPixmap(void);

  public:
    explicit ColorChooserButton(QWidget *parent = nullptr);
    ~ColorChooserButton();

    QColor
    getColor(void)
    {
      return this->current;
    }

    void
    getColor(QColor &color)
    {
      color = this->current;
    }

    void
    setColor(QColor color)
    {
      if (this->current != color) {
        this->current = color;
        emit colorChanged(color);
      }

      this->resetPixmap();
    }

  private:
    Ui::ColorChooserButton *ui;

  public slots:
    void onClicked(void);

  signals:
    void colorChanged(QColor);
};

#endif // COLORCHOOSERBUTTON_H
