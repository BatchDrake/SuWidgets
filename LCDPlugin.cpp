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
#include "LCD.h"
#include "LCDPlugin.h"

#include <QtPlugin>

LCDPlugin::LCDPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void LCDPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool LCDPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *LCDPlugin::createWidget(QWidget *parent)
{
  return new LCD(parent);
}

QString LCDPlugin::name() const
{
  return QLatin1String("LCD");
}

QString LCDPlugin::group() const
{
  return QLatin1String("");
}

QIcon LCDPlugin::icon() const
{
  return QIcon();
}

QString LCDPlugin::toolTip() const
{
  return QLatin1String("");
}

QString LCDPlugin::whatsThis() const
{
  return QLatin1String("");
}

bool LCDPlugin::isContainer() const
{
  return false;
}

QString LCDPlugin::domXml() const
{
  return QLatin1String("<widget class=\"LCD\" name=\"lCD\">\n</widget>\n");
}

QString LCDPlugin::includeFile() const
{
  return QLatin1String("LCD.h");
}

