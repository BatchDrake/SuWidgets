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
#include "TVDisplay.h"
#include "TVDisplayPlugin.h"

#include <QtPlugin>

TVDisplayPlugin::TVDisplayPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void TVDisplayPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool TVDisplayPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *TVDisplayPlugin::createWidget(QWidget *parent)
{
  return new TVDisplay(parent);
}

QString TVDisplayPlugin::name() const
{
  return QLatin1String("TVDisplay");
}

QString TVDisplayPlugin::group() const
{
  return QLatin1String("");
}

QIcon TVDisplayPlugin::icon() const
{
  return QIcon();
}

QString TVDisplayPlugin::toolTip() const
{
  return QLatin1String("");
}

QString TVDisplayPlugin::whatsThis() const
{
  return QLatin1String("");
}

bool TVDisplayPlugin::isContainer() const
{
  return false;
}

QString TVDisplayPlugin::domXml() const
{
  return QLatin1String("<widget class=\"TVDisplay\" name=\"tvDisplay\">\n</widget>\n");
}

QString TVDisplayPlugin::includeFile() const
{
  return QLatin1String("TVDisplay.h");
}

