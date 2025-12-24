//
//    TimeSpinBoxPlugin.cpp
//    Copyright (C) 2025 Gonzalo José Carracedo Carballal
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

#include "TimeSpinBox.h"
#include "TimeSpinBoxPlugin.h"

#include <QtPlugin>

TimeSpinBoxPlugin::TimeSpinBoxPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void TimeSpinBoxPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool TimeSpinBoxPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *TimeSpinBoxPlugin::createWidget(QWidget *parent)
{
  return new TimeSpinBox(parent);
}

QString TimeSpinBoxPlugin::name() const
{
  return QLatin1String("TimeSpinBox");
}

QString TimeSpinBoxPlugin::group() const
{
  return QLatin1String("");
}

QIcon TimeSpinBoxPlugin::icon() const
{
  return QIcon();
}

QString TimeSpinBoxPlugin::toolTip() const
{
  return QLatin1String("");
}

QString TimeSpinBoxPlugin::whatsThis() const
{
  return QLatin1String("");
}

bool TimeSpinBoxPlugin::isContainer() const
{
  return false;
}

QString TimeSpinBoxPlugin::domXml() const
{
  return QLatin1String("<widget class=\"TimeSpinBox\" name=\"timeSpinBox\">\n</widget>\n");
}

QString TimeSpinBoxPlugin::includeFile() const
{
  return QLatin1String("TimeSpinBox.h");
}

