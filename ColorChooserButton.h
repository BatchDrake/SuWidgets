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
      this->current = color;
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
