//
//    SciSpinBoxPlugin.cpp
//    Copyright (C) 2023 Gonzalo José Carracedo Carballal
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
#include "SciSpinBox.h"
#include "SciSpinBoxPlugin.h"

#include <QtPlugin>

SciSpinBoxPlugin::SciSpinBoxPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void SciSpinBoxPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool SciSpinBoxPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *SciSpinBoxPlugin::createWidget(QWidget *parent)
{
  return new SciSpinBox(parent);
}

QString SciSpinBoxPlugin::name() const
{
  return QLatin1String("SciSpinBox");
}

QString SciSpinBoxPlugin::group() const
{
  return QLatin1String("");
}

QIcon SciSpinBoxPlugin::icon() const
{
  return QIcon();
}

QString SciSpinBoxPlugin::toolTip() const
{
  return QLatin1String("");
}

QString SciSpinBoxPlugin::whatsThis() const
{
  return QLatin1String("");
}

bool SciSpinBoxPlugin::isContainer() const
{
  return false;
}

QString SciSpinBoxPlugin::domXml() const
{
  return QLatin1String("<widget class=\"SciSpinBox\" name=\"sciSpinBox\">\n</widget>\n");
}

QString SciSpinBoxPlugin::includeFile() const
{
  return QLatin1String("SciSpinBox.h");
}

