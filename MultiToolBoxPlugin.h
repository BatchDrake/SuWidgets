//
//    filename: description
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
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
#ifndef MULTITOOLBOXPLUGIN_H
#define MULTITOOLBOXPLUGIN_H

#include <QtUiPlugin/QDesignerCustomWidgetInterface>
#include <QExtensionManager>
#include <QDesignerPropertySheetExtension>
#include <QDesignerFormEditorInterface>
#include <QExtensionFactory>
#include <QDesignerContainerExtension>

class MultiToolBox;

class MultiToolBoxContainerExtension: public QObject,
                                         public QDesignerContainerExtension
{
  Q_OBJECT
  Q_INTERFACES(QDesignerContainerExtension)

public:
  explicit MultiToolBoxContainerExtension(MultiToolBox *widget, QObject *parent);

  void addWidget(QWidget *widget) override;
  int count() const override;
  int currentIndex() const override;
  void insertWidget(int index, QWidget *widget) override;
  void remove(int index) override;
  void setCurrentIndex(int index) override;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  bool canAddWidget() const override;
  bool canRemove(int) const override;
#endif

  QWidget *widget(int index) const override;

private:
  MultiToolBox *myWidget;
};

class MultiToolBoxExtensionFactory: public QExtensionFactory
{
  Q_OBJECT

public:
  explicit MultiToolBoxExtensionFactory(QExtensionManager *parent = nullptr);

protected:
  QObject *createExtension(QObject *object, const QString &iid, QObject *parent) const override;
};


class MultiToolBoxPlugin : public QObject, public QDesignerCustomWidgetInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetInterface)

public:
  MultiToolBoxPlugin(QObject *parent = 0);

  bool isContainer() const;
  bool isInitialized() const;
  QIcon icon() const;
  QString domXml() const;
  QString group() const;
  QString includeFile() const;
  QString name() const;
  QString toolTip() const;
  QString whatsThis() const;
  QWidget *createWidget(QWidget *parent);
  void initialize(QDesignerFormEditorInterface *core);

  public slots:
    void currentIndexChanged(int index);
    void pageTitleChanged(QString);


private:
  bool m_initialized;
};

#endif
