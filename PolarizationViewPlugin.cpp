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
#include "PolarizationView.h"
#include "PolarizationViewPlugin.h"

#include <QtPlugin>

PolarizationViewPlugin::PolarizationViewPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void PolarizationViewPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool PolarizationViewPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *PolarizationViewPlugin::createWidget(QWidget *parent)
{
  return new PolarizationView(parent);
}

QString PolarizationViewPlugin::name() const
{
  return QLatin1String("PolarizationView");
}

QString PolarizationViewPlugin::group() const
{
  return QLatin1String("");
}

QIcon PolarizationViewPlugin::icon() const
{
  return QIcon(":/icons/open_icon.png");
}

QString PolarizationViewPlugin::toolTip() const
{
  return QLatin1String("");
}

QString PolarizationViewPlugin::whatsThis() const
{
  return QLatin1String("Represents the polarization ellipsoid of two Jones vectors");
}

bool PolarizationViewPlugin::isContainer() const
{
  return false;
}

QString PolarizationViewPlugin::domXml() const
{
  return QLatin1String("<widget class=\"PolarizationView\" name=\"polarizationView\">\n</widget>\n");
}

QString PolarizationViewPlugin::includeFile() const
{
  return QLatin1String("PolarizationView.h");
}

