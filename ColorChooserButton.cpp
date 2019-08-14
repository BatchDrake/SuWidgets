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
  QColor color = QColorDialog::getColor(Qt::yellow, this );
  if (color.isValid())
    this->setColor(color);
}

ColorChooserButton::~ColorChooserButton()
{
  delete ui;
}
