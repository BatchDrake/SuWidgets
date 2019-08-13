#include "ColorChooserButton.h"
#include "ui_ColorChooserButton.h"

ColorChooserButton::ColorChooserButton(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::ColorChooserButton)
{
  ui->setupUi(this);
}

ColorChooserButton::~ColorChooserButton()
{
  delete ui;
}
