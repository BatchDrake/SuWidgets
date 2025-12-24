//
//    WaterfallPlugin.cpp
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

#include "Waterfall.h"
#include "WaterfallPlugin.h"

#include <QtPlugin>

WaterfallPlugin::WaterfallPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void WaterfallPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool WaterfallPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *WaterfallPlugin::createWidget(QWidget *parent)
{
  return new Waterfall(parent);
}

QString WaterfallPlugin::name() const
{
  return QLatin1String("Waterfall");
}

QString WaterfallPlugin::group() const
{
  return QLatin1String("");
}

QIcon WaterfallPlugin::icon() const
{
  return QIcon();
}

QString WaterfallPlugin::toolTip() const
{
  return QLatin1String("");
}

QString WaterfallPlugin::whatsThis() const
{
  return QLatin1String("");
}

bool WaterfallPlugin::isContainer() const
{
  return false;
}

QString WaterfallPlugin::domXml() const
{
  return QLatin1String("<widget class=\"Waterfall\" name=\"Waterfall\">\n</widget>\n");
}

QString WaterfallPlugin::includeFile() const
{
  return QLatin1String("Waterfall.h");
}

