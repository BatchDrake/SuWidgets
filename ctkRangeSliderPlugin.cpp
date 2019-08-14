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
#include "ctkRangeSlider.h"
#include "ctkRangeSliderPlugin.h"

#include <QtPlugin>

ctkRangeSliderPlugin::ctkRangeSliderPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void ctkRangeSliderPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool ctkRangeSliderPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *ctkRangeSliderPlugin::createWidget(QWidget *parent)
{
  return new ctkRangeSlider(parent);
}

QString ctkRangeSliderPlugin::name() const
{
  return QLatin1String("ctkRangeSlider");
}

QString ctkRangeSliderPlugin::group() const
{
  return QLatin1String("");
}

QIcon ctkRangeSliderPlugin::icon() const
{
  return QIcon();
}

QString ctkRangeSliderPlugin::toolTip() const
{
  return QLatin1String("");
}

QString ctkRangeSliderPlugin::whatsThis() const
{
  return QLatin1String("");
}

bool ctkRangeSliderPlugin::isContainer() const
{
  return false;
}

QString ctkRangeSliderPlugin::domXml() const
{
  return QLatin1String("<widget class=\"ctkRangeSlider\" name=\"ctkRangeSlider\">\n</widget>\n");
}

QString ctkRangeSliderPlugin::includeFile() const
{
  return QLatin1String("ctkRangeSlider.h");
}

