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
#include "EgaView.h"
#include "EgaViewPlugin.h"

#include <QtPlugin>

EgaViewPlugin::EgaViewPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void EgaViewPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool EgaViewPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *EgaViewPlugin::createWidget(QWidget *parent)
{
  return new EgaView(parent);
}

QString EgaViewPlugin::name() const
{
  return QLatin1String("EgaView");
}

QString EgaViewPlugin::group() const
{
  return QLatin1String("");
}

QIcon EgaViewPlugin::icon() const
{
  return QIcon(":/icons/open_icon.png");
}

QString EgaViewPlugin::toolTip() const
{
  return QLatin1String("");
}

QString EgaViewPlugin::whatsThis() const
{
  return QLatin1String("EgaView widget");
}

bool EgaViewPlugin::isContainer() const
{
  return false;
}

QString EgaViewPlugin::domXml() const
{
  return QLatin1String("<widget class=\"EgaView\" name=\"egaView\">\n</widget>\n");
}

QString EgaViewPlugin::includeFile() const
{
  return QLatin1String("EgaView.h");
}

