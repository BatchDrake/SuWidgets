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
  child(child)
{
  this->child->setProperty("collapsed", QVariant::fromValue<bool>(!visible));
  this->setName(name);
}

void
MultiToolBoxItem::setName(QString const &name)
{
  if (name != this->child->windowTitle())
    this->child->setProperty("windowTitle", QVariant::fromValue(name));

  this->name = name;
}

MultiToolBoxItem::~MultiToolBoxItem(void)
{
}

void
MultiToolBoxItem::setVisible(bool visible)
{
  bool propVisible = this->isVisible();

  if (propVisible != visible) {
    this->child->setProperty("collapsed", QVariant::fromValue<bool>(!visible));
    emit stateChanged();
  }
}

bool
MultiToolBoxItem::isVisible(void) const
{
  return !this->child->property("collapsed").value<bool>();
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
      button->setText(" ▼ " + p->getChild()->windowTitle());
    else
      button->setText(" ▶ " + p->getChild()->windowTitle());

    p->getChild()->setVisible(p->isVisible());
  }
}

int
MultiToolBox::addItem(MultiToolBoxItem *item)
{
  QPushButton *button = nullptr;

  if (this->itemLayout == nullptr) {
    this->itemLayout = new QVBoxLayout(this->ui->scrollAreaWidgetContents);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    this->itemLayout->setMargin(0);
#endif
    this->itemLayout->setSpacing(1);
    this->itemLayout->setAlignment(Qt::AlignTop);
  }

  button = new QPushButton();
  button->setProperty(
        "multiIndex",
        QVariant::fromValue<int>(this->itemList.size()));

  item->getChild()->setProperty(
        "multiIndex",
        QVariant::fromValue<int>(this->itemList.size()));

  item->getChild()->installEventFilter(this);

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

  connect(
        item->getChild(),
        &QWidget::windowTitleChanged,
        this,
        &MultiToolBox::pageWindowTitleChanged);

  this->refreshVisibility();

  return this->itemList.size() - 1;
}

void
MultiToolBox::addPage(QWidget *page)
{
  int index;

  index = this->addItem(new MultiToolBoxItem(page->windowTitle(), page));

  this->setCurrentIndex(index);
}


int
MultiToolBox::currentIndex(void) const
{
  return this->index;
}

void
MultiToolBox::setCurrentIndex(int index)
{
  int i;

  if (index != this->index) {
    this->index = index;

    for (i = 0; i < this->itemList.size(); ++i)
      this->itemList[i]->setVisible(i == index);

    if (index != -1)
      emit currentIndexChanged(index);
  }
}

QString
MultiToolBox::pageTitle(void) const
{
  MultiToolBoxItem *item;

  if ((item = this->itemAt(this->index)) == nullptr)
    return "(no page)";

  return item->getName();
}

void
MultiToolBox::setPageTitle(QString name)
{
  MultiToolBoxItem *item;

  if ((item = this->itemAt(this->index)) == nullptr)
    return;

  item->setName(name);
  this->refreshVisibility();

  emit pageTitleChanged(name);
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
  QVariant index = button->property("multiIndex");
  MultiToolBoxItem *item = this->itemAt(index.value<int>());

  if (item != nullptr) {
    item->setVisible(!item->isVisible());

    if (item->isVisible())
      this->index = index.value<int>();
  }
}

void
MultiToolBox::onStateChanged(void)
{
  this->refreshVisibility();
}

void MultiToolBox::pageWindowTitleChanged()
{
  MultiToolBoxItem *item = this->itemAt(this->index);

  if (item != nullptr)
    this->setPageTitle(item->getChild()->windowTitle());

}

bool
MultiToolBox::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::DynamicPropertyChange) {
    QDynamicPropertyChangeEvent *const propEvent = static_cast<QDynamicPropertyChangeEvent*>(event);
    QString propName = propEvent->propertyName();
    if (propName == "collapsed") {
      int index    = obj->property("multiIndex").value<int>();
      bool visible = !obj->property("collapsed").value<bool>();

      if (visible)
        this->showItem(index);
      else
        this->hideItem(index);

      this->refreshVisibility();
    }
  }

  return QObject::eventFilter(obj, event);
}

