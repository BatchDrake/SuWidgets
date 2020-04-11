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
#include "FrequencySpinBox.h"
#include "FrequencySpinBoxPlugin.h"

#include <QtPlugin>

FrequencySpinBoxPlugin::FrequencySpinBoxPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void FrequencySpinBoxPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool FrequencySpinBoxPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *FrequencySpinBoxPlugin::createWidget(QWidget *parent)
{
  return new FrequencySpinBox(parent);
}

QString FrequencySpinBoxPlugin::name() const
{
  return QLatin1String("FrequencySpinBox");
}

QString FrequencySpinBoxPlugin::group() const
{
  return QLatin1String("");
}

QIcon FrequencySpinBoxPlugin::icon() const
{
  return QIcon(":/icons/open_icon.png");
}

QString FrequencySpinBoxPlugin::toolTip() const
{
  return QLatin1String("");
}

QString FrequencySpinBoxPlugin::whatsThis() const
{
  return QLatin1String("Button that allows you to pick a color");
}

bool FrequencySpinBoxPlugin::isContainer() const
{
  return false;
}

QString FrequencySpinBoxPlugin::domXml() const
{
  return QLatin1String("<widget class=\"FrequencySpinBox\" name=\"frequencySpinBox\">\n</widget>\n");
}

QString FrequencySpinBoxPlugin::includeFile() const
{
  return QLatin1String("FrequencySpinBox.h");
}

