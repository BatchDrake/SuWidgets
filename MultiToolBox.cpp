//
//    MultiToolBox.cpp: Toolbox that enables multiple opened sections
//    Copyright (C) 2021 Gonzalo José Carracedo Carballal
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
#include "MultiToolBox.h"
#include "ui_MultiToolBox.h"
#include <QLayout>
#include <QLayoutItem>

#include <QPushButton>

MultiToolBoxItem::MultiToolBoxItem(
    QString const &name,
    QWidget *child,
    bool visible,
    QObject *parent) : QObject(parent),
  name(name),
  child(child),
  visible(visible)
{

}

MultiToolBoxItem::~MultiToolBoxItem(void)
{
  if (this->child->parent() == nullptr)
    this->child->deleteLater();
}

void
MultiToolBoxItem::setVisible(bool visible)
{
  if (this->visible != visible) {
    this->visible = visible;
    emit stateChanged();
  }
}

bool
MultiToolBoxItem::isVisible(void) const
{
  return this->visible;
}

QWidget *
MultiToolBoxItem::getChild(void) const
{
  return this->child;
}

QString
MultiToolBoxItem::getName(void) const
{
  return this->name;
}


///////////////////////////////// MultiToolBox /////////////////////////////////
MultiToolBox::MultiToolBox(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::MultiToolBox)
{
  ui->setupUi(this);
}

MultiToolBox::~MultiToolBox()
{
  delete ui;
}

void
MultiToolBox::refreshVisibility(void)
{
  int i;

  for (i = 0; i < this->itemList.count(); ++i) {
    QPushButton *button = this->buttonList[i];
    MultiToolBoxItem *p = this->itemList[i];

    if (p->isVisible())
      button->setText(" ▼ " + p->getName());
    else
      button->setText(" ▶ " + p->getName());

    p->getChild()->setVisible(p->isVisible());
  }
}

int
MultiToolBox::addItem(MultiToolBoxItem *item)
{
  QPushButton *button = nullptr;

  if (this->itemLayout == nullptr) {
    this->itemLayout = new QVBoxLayout(this->ui->scrollAreaWidgetContents);
    this->itemLayout->setMargin(0);
    this->itemLayout->setSpacing(1);
    this->itemLayout->setAlignment(Qt::AlignTop);
  }

  button = new QPushButton();
  button->setProperty("index", QVariant::fromValue<int>(this->itemList.size()));
  button->setStyleSheet("text-align: left; font-weight: bold");
  button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

  this->itemLayout->addWidget(button);
  this->itemLayout->addWidget(item->getChild());

  this->itemList.push_back(item);
  this->buttonList.push_back(button);

  item->setParent(this);

  connect(
        button,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onToggleVisibility(void)));

  connect(
        item,
        SIGNAL(stateChanged(void)),
        this,
        SLOT(onStateChanged(void)));

  this->refreshVisibility();

  return this->itemList.size() - 1;
}

bool
MultiToolBox::showItem(int index)
{
  MultiToolBoxItem *item = this->itemAt(index);

  if (item == nullptr)
    return false;

  item->getChild()->setVisible(true);

  return true;
}

bool
MultiToolBox::hideItem(int index)
{
  MultiToolBoxItem *item = this->itemAt(index);

  if (item == nullptr)
    return false;

  item->getChild()->setVisible(false);

  return true;
}

int
MultiToolBox::count(void) const
{
  return this->itemList.size();
}

MultiToolBoxItem *
MultiToolBox::itemAt(int index) const
{
  if (index < 0 || index >= this->itemList.size())
    return nullptr;

  return this->itemList[index];
}

//////////////////////////////////// Slots ////////////////////////////////////
void
MultiToolBox::onToggleVisibility(void)
{
  QPushButton *button = static_cast<QPushButton *>(QObject::sender());
  QVariant index = button->property("index");
  MultiToolBoxItem *item = this->itemAt(index.value<int>());

  if (item != nullptr)
    item->setVisible(!item->isVisible());
}

void
MultiToolBox::onStateChanged(void)
{
  this->refreshVisibility();
}
