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
#include "PhaseView.h"
#include "PhaseViewPlugin.h"

#include <QtPlugin>

PhaseViewPlugin::PhaseViewPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void PhaseViewPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool PhaseViewPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *PhaseViewPlugin::createWidget(QWidget *parent)
{
  return new PhaseView(parent);
}

QString PhaseViewPlugin::name() const
{
  return QLatin1String("PhaseView");
}

QString PhaseViewPlugin::group() const
{
  return QLatin1String("");
}

QIcon PhaseViewPlugin::icon() const
{
  return QIcon(":/icons/open_icon.png");
}

QString PhaseViewPlugin::toolTip() const
{
  return QLatin1String("");
}

QString PhaseViewPlugin::whatsThis() const
{
  return QLatin1String("Represents the phase of complex signals");
}

bool PhaseViewPlugin::isContainer() const
{
  return false;
}

QString PhaseViewPlugin::domXml() const
{
  return QLatin1String("<widget class=\"PhaseView\" name=\"phaseView\">\n</widget>\n");
}

QString PhaseViewPlugin::includeFile() const
{
  return QLatin1String("PhaseView.h");
}

