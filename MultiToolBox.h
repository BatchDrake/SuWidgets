//
//    MultiToolBox.h: Description
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
#ifndef MULTITOOLBOX_H
#define MULTITOOLBOX_H

#include <QWidget>

namespace Ui {
  class MultiToolBox;
}

class QPushButton;
class QLayout;

class MultiToolBoxItem : public QObject
{
    Q_OBJECT

    QString name;
    QWidget *child = nullptr;
    bool visible = true;

  public:
    explicit MultiToolBoxItem(
        QString const &name,
        QWidget *child,
        bool visible = true,
        QObject *parent = nullptr);
    ~MultiToolBoxItem(void);

    void setVisible(bool);
    bool isVisible(void) const;
    QWidget *getChild(void) const;
    QString getName(void) const;

  signals:
    void stateChanged(void);
};

class MultiToolBox : public QWidget
{
    Q_OBJECT

    QList<MultiToolBoxItem *> itemList;
    QList<QPushButton *> buttonList;

    QLayout *itemLayout = nullptr;

    void refreshVisibility(void);

  public:
    explicit MultiToolBox(QWidget *parent = nullptr);
    ~MultiToolBox();

    int addItem(MultiToolBoxItem *item);

    bool showItem(int);
    bool hideItem(int);

    int count(void) const;
    MultiToolBoxItem *itemAt(int) const;

  public slots:
    void onToggleVisibility(void);
    void onStateChanged(void);

  private:
    Ui::MultiToolBox *ui;
};

#endif // MULTITOOLBOX_H
