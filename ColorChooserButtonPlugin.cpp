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
#include "ColorChooserButton.h"
#include "ColorChooserButtonPlugin.h"

#include <QtPlugin>

ColorChooserButtonPlugin::ColorChooserButtonPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void ColorChooserButtonPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool ColorChooserButtonPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *ColorChooserButtonPlugin::createWidget(QWidget *parent)
{
  return new ColorChooserButton(parent);
}

QString ColorChooserButtonPlugin::name() const
{
  return QLatin1String("ColorChooserButton");
}

QString ColorChooserButtonPlugin::group() const
{
  return QLatin1String("");
}

QIcon ColorChooserButtonPlugin::icon() const
{
  return QIcon(":/icons/open_icon.png");
}

QString ColorChooserButtonPlugin::toolTip() const
{
  return QLatin1String("");
}

QString ColorChooserButtonPlugin::whatsThis() const
{
  return QLatin1String("Button that allows you to pick a color");
}

bool ColorChooserButtonPlugin::isContainer() const
{
  return false;
}

QString ColorChooserButtonPlugin::domXml() const
{
  return QLatin1String("<widget class=\"ColorChooserButton\" name=\"ColorChooserButton\">\n</widget>\n");
}

QString ColorChooserButtonPlugin::includeFile() const
{
  return QLatin1String("ColorChooserButton.h");
}

