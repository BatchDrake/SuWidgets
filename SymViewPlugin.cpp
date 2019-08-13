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
#include "SymView.h"
#include "SymViewPlugin.h"

#include <QtPlugin>

SymViewPlugin::SymViewPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void SymViewPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool SymViewPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *SymViewPlugin::createWidget(QWidget *parent)
{
  return new SymView(parent);
}

QString SymViewPlugin::name() const
{
  return QLatin1String("SymView");
}

QString SymViewPlugin::group() const
{
  return QLatin1String("");
}

QIcon SymViewPlugin::icon() const
{
  return QIcon();
}

QString SymViewPlugin::toolTip() const
{
  return QLatin1String("");
}

QString SymViewPlugin::whatsThis() const
{
  return QLatin1String("");
}

bool SymViewPlugin::isContainer() const
{
  return false;
}

QString SymViewPlugin::domXml() const
{
  return QLatin1String("<widget class=\"SymView\" name=\"symView\">\n</widget>\n");
}

QString SymViewPlugin::includeFile() const
{
  return QLatin1String("SymView.h");
}

