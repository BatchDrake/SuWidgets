//
//    MultiToolBox.h: Toolbox that enables multiple opened sections
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
    void setName(QString const &name);

  signals:
    void stateChanged(void);
};

class MultiToolBox : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex)
    Q_PROPERTY(QString pageTitle READ pageTitle WRITE setPageTitle STORED false)

    QList<MultiToolBoxItem *> itemList;
    QList<QPushButton *> buttonList;

    QLayout *itemLayout = nullptr;

    // Fake index
    int index = -1;

    void refreshVisibility(void);

  public:
    explicit MultiToolBox(QWidget *parent = nullptr);
    ~MultiToolBox();

    int addItem(MultiToolBoxItem *item);

    bool showItem(int);
    bool hideItem(int);

    QString pageTitle(void) const;
    int currentIndex(void) const;

    int count(void) const;
    MultiToolBoxItem *itemAt(int) const;

  signals:
    void currentIndexChanged(int index);
    void pageTitleChanged(QString);

  public slots:
    void onToggleVisibility(void);
    void onStateChanged(void);

    void addPage(QWidget *page);
    void setCurrentIndex(int);
    void setPageTitle(QString);
    void pageWindowTitleChanged();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

  private:
    Ui::MultiToolBox *ui;
};

#endif // MULTITOOLBOX_H
