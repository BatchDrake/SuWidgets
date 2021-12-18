//
//    MultiToolBoxPlugin.cpp: Toolbox with multiple simulatenously opened
//    widgets
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
#include "MultiToolBoxPlugin.h"
#include <QDesignerFormWindowInterface>
#include <QtPlugin>


/////////////////////// Container extension impl //////////////////////////////
MultiToolBoxContainerExtension::MultiToolBoxContainerExtension(MultiToolBox *widget,
                                                                     QObject *parent)
    : QObject(parent)
    , myWidget(widget)
{
}

void
MultiToolBoxContainerExtension::addWidget(QWidget *widget)
{
  myWidget->addPage(widget);
}

int
MultiToolBoxContainerExtension::count() const
{
  return myWidget->count();
}

int
MultiToolBoxContainerExtension::currentIndex() const
{
  return myWidget->currentIndex();
}

void
MultiToolBoxContainerExtension::insertWidget(int index, QWidget *widget)
{
  if (index == myWidget->count())
    this->addWidget(widget);
  else
    fprintf(stderr, "Adding pages in the middle not yet supported\n");
}

void
MultiToolBoxContainerExtension::remove(int)
{
  fprintf(stderr, "Removing pages not yet supported\n");
}

void
MultiToolBoxContainerExtension::setCurrentIndex(int index)
{
  myWidget->setCurrentIndex(index);
}

QWidget *
MultiToolBoxContainerExtension::widget(int index) const
{
  auto p = myWidget->itemAt(index);

  if (p != nullptr)
    return p->getChild();

  return nullptr;
}

/////////////////////// Container extension factory impl ///////////////////////

MultiToolBoxExtensionFactory::MultiToolBoxExtensionFactory(QExtensionManager *parent)
    : QExtensionFactory(parent)
{
}

QObject *MultiToolBoxExtensionFactory::createExtension(QObject *object,
                                                          const QString &iid,
                                                          QObject *parent) const
{
  MultiToolBox *widget = qobject_cast<MultiToolBox*>(object);

  if (widget && (iid == Q_TYPEID(QDesignerContainerExtension)))
    return new MultiToolBoxContainerExtension(widget, parent);
  return nullptr;
}

MultiToolBoxPlugin::MultiToolBoxPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void
MultiToolBoxPlugin::initialize(QDesignerFormEditorInterface *formEditor)
{
  if (m_initialized)
    return;

  QExtensionManager *manager = formEditor->extensionManager();

  QExtensionFactory *factory = new MultiToolBoxExtensionFactory(manager);

  Q_ASSERT(manager != 0);
  manager->registerExtensions(factory, Q_TYPEID(QDesignerContainerExtension));

  m_initialized = true;
}

bool
MultiToolBoxPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *
MultiToolBoxPlugin::createWidget(QWidget *parent)
{
  QWidget *widget = new MultiToolBox(parent);

  connect(
        widget,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(currentIndexChanged(int)));

  connect(
        widget,
        SIGNAL(pageTitleChanged(QString)),
        this,
        SLOT(pageTitleChanged(QString)));

  return widget;
}

QString
MultiToolBoxPlugin::name() const
{
  return QLatin1String("MultiToolBox");
}

QString
MultiToolBoxPlugin::group() const
{
  return QLatin1String("");
}

QIcon
MultiToolBoxPlugin::icon() const
{
  return QIcon();
}

QString
MultiToolBoxPlugin::toolTip() const
{
  return QLatin1String("");
}

QString
MultiToolBoxPlugin::whatsThis() const
{
  return QLatin1String("");
}

bool
MultiToolBoxPlugin::isContainer() const
{
  return true;
}

QString
MultiToolBoxPlugin::domXml() const
{
  return QLatin1String("\
<ui language=\"c++\">\
  <widget class=\"MultiToolBox\" name=\"multiToolBox\">\
      <widget class=\"QWidget\" name=\"multiToolBoxPage\" />\
  </widget>\
  <customwidgets>\
      <customwidget>\
          <class>MultiToolBox</class>\
          <extends>QWidget</extends>\
          <addpagemethod>addPage</addpagemethod>\
      </customwidget>\
  </customwidgets>\
</ui>");
}

QString
MultiToolBoxPlugin::includeFile() const
{
  return QLatin1String("MultiToolBox.h");
}

void
MultiToolBoxPlugin::currentIndexChanged(int)
{
  MultiToolBox *widget = qobject_cast<MultiToolBox*>(sender());

  if (widget != nullptr) {
    QDesignerFormWindowInterface *form = QDesignerFormWindowInterface::findFormWindow(widget);
    if (form != nullptr)
      form->emitSelectionChanged();
  }
}

void
MultiToolBoxPlugin::pageTitleChanged(QString)
{
  MultiToolBox *widget = qobject_cast<MultiToolBox*>(sender());

  if (widget != nullptr) {
    MultiToolBoxItem *item = widget->itemAt(widget->currentIndex());
    if (item != nullptr) {
      QWidget *page = item->getChild();
      QDesignerFormWindowInterface *form;
      form = QDesignerFormWindowInterface::findFormWindow(widget);

      if (form != nullptr) {
        QDesignerFormEditorInterface *editor = form->core();
        QExtensionManager *manager = editor->extensionManager();
        QDesignerPropertySheetExtension *sheet;
        sheet = qt_extension<QDesignerPropertySheetExtension*>(manager, page);
        const int propertyIndex = sheet->indexOf(QLatin1String("windowTitle"));
        sheet->setChanged(propertyIndex, true);
      }
    }
  }
}
