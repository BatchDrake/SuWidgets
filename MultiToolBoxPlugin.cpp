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

#include <QtPlugin>

MultiToolBoxPlugin::MultiToolBoxPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void MultiToolBoxPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool MultiToolBoxPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *MultiToolBoxPlugin::createWidget(QWidget *parent)
{
  return new MultiToolBox(parent);
}

QString MultiToolBoxPlugin::name() const
{
  return QLatin1String("MultiToolBox");
}

QString MultiToolBoxPlugin::group() const
{
  return QLatin1String("");
}

QIcon MultiToolBoxPlugin::icon() const
{
  return QIcon();
}

QString MultiToolBoxPlugin::toolTip() const
{
  return QLatin1String("");
}

QString MultiToolBoxPlugin::whatsThis() const
{
  return QLatin1String("");
}

bool MultiToolBoxPlugin::isContainer() const
{
  return false;
}

QString MultiToolBoxPlugin::domXml() const
{
  return QLatin1String("<widget class=\"MultiToolBox\" name=\"multiToolBox\">\n</widget>\n");
}

QString MultiToolBoxPlugin::includeFile() const
{
  return QLatin1String("MultiToolBox.h");
}

