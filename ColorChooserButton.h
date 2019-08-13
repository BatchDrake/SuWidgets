#ifndef COLORCHOOSERBUTTON_H
#define COLORCHOOSERBUTTON_H

#include <QWidget>

namespace Ui {
  class ColorChooserButton;
}

class ColorChooserButton : public QWidget
{
    Q_OBJECT

  public:
    explicit ColorChooserButton(QWidget *parent = nullptr);
    ~ColorChooserButton();

  private:
    Ui::ColorChooserButton *ui;
};

#endif // COLORCHOOSERBUTTON_H
