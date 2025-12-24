//
//    ConstellationPlugin.cpp: Constellation widget
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
#include "Constellation.h"
#include "ConstellationPlugin.h"

#include <QtPlugin>

ConstellationPlugin::ConstellationPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void ConstellationPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool ConstellationPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *ConstellationPlugin::createWidget(QWidget *parent)
{
  return new Constellation(parent);
}

QString ConstellationPlugin::name() const
{
  return QLatin1String("Constellation");
}

QString ConstellationPlugin::group() const
{
  return QLatin1String("");
}

QIcon ConstellationPlugin::icon() const
{
  return QIcon(":/icons/open_icon.png");
}

QString ConstellationPlugin::toolTip() const
{
  return QLatin1String("");
}

QString ConstellationPlugin::whatsThis() const
{
  return QLatin1String("Constellation widget for phase-modulated signals");
}

bool ConstellationPlugin::isContainer() const
{
  return false;
}

QString ConstellationPlugin::domXml() const
{
  return QLatin1String("<widget class=\"Constellation\" name=\"constellation\">\n</widget>\n");
}

QString ConstellationPlugin::includeFile() const
{
  return QLatin1String("Constellation.h");
}

