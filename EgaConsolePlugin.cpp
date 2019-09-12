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
#include "EgaConsole.h"
#include "EgaConsolePlugin.h"

#include <QtPlugin>

EgaConsolePlugin::EgaConsolePlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void EgaConsolePlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool EgaConsolePlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *EgaConsolePlugin::createWidget(QWidget *parent)
{
  return new EgaConsole(parent);
}

QString EgaConsolePlugin::name() const
{
  return QLatin1String("EgaConsole");
}

QString EgaConsolePlugin::group() const
{
  return QLatin1String("");
}

QIcon EgaConsolePlugin::icon() const
{
  return QIcon(":/icons/open_icon.png");
}

QString EgaConsolePlugin::toolTip() const
{
  return QLatin1String("");
}

QString EgaConsolePlugin::whatsThis() const
{
  return QLatin1String("EgaConsole widget");
}

bool EgaConsolePlugin::isContainer() const
{
  return false;
}

QString EgaConsolePlugin::domXml() const
{
  return QLatin1String("<widget class=\"EgaConsole\" name=\"egaConsole\">\n</widget>\n");
}

QString EgaConsolePlugin::includeFile() const
{
  return QLatin1String("EgaConsole.h");
}

