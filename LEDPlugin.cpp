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
#include "LED.h"
#include "LEDPlugin.h"

#include <QtPlugin>

LEDPlugin::LEDPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void LEDPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool LEDPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *LEDPlugin::createWidget(QWidget *parent)
{
  return new LED(parent);
}

QString LEDPlugin::name() const
{
  return QLatin1String("LED");
}

QString LEDPlugin::group() const
{
  return QLatin1String("");
}

QIcon LEDPlugin::icon() const
{
  return QIcon();
}

QString LEDPlugin::toolTip() const
{
  return QLatin1String("");
}

QString LEDPlugin::whatsThis() const
{
  return QLatin1String("");
}

bool LEDPlugin::isContainer() const
{
  return false;
}

QString LEDPlugin::domXml() const
{
  return QLatin1String("<widget class=\"LED\" name=\"led\">\n</widget>\n");
}

QString LEDPlugin::includeFile() const
{
  return QLatin1String("LED.h");
}

