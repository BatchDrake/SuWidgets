//
//    HistogramPlugin.cpp: Simple symbol histogram
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
#include "Histogram.h"
#include "HistogramPlugin.h"

#include <QtPlugin>

HistogramPlugin::HistogramPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void HistogramPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool HistogramPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *HistogramPlugin::createWidget(QWidget *parent)
{
  return new Histogram(parent);
}

QString HistogramPlugin::name() const
{
  return QLatin1String("Histogram");
}

QString HistogramPlugin::group() const
{
  return QLatin1String("");
}

QIcon HistogramPlugin::icon() const
{
  return QIcon();
}

QString HistogramPlugin::toolTip() const
{
  return QLatin1String("");
}

QString HistogramPlugin::whatsThis() const
{
  return QLatin1String("");
}

bool HistogramPlugin::isContainer() const
{
  return false;
}

QString HistogramPlugin::domXml() const
{
  return QLatin1String("<widget class=\"Histogram\" name=\"histogram\">\n</widget>\n");
}

QString HistogramPlugin::includeFile() const
{
  return QLatin1String("Histogram.h");
}

