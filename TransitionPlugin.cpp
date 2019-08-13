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
#include "Transition.h"
#include "TransitionPlugin.h"

#include <QtPlugin>

TransitionPlugin::TransitionPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void TransitionPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool TransitionPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *TransitionPlugin::createWidget(QWidget *parent)
{
  return new Transition(parent);
}

QString TransitionPlugin::name() const
{
  return QLatin1String("Transition");
}

QString TransitionPlugin::group() const
{
  return QLatin1String("");
}

QIcon TransitionPlugin::icon() const
{
  return QIcon();
}

QString TransitionPlugin::toolTip() const
{
  return QLatin1String("");
}

QString TransitionPlugin::whatsThis() const
{
  return QLatin1String("");
}

bool TransitionPlugin::isContainer() const
{
  return false;
}

QString TransitionPlugin::domXml() const
{
  return QLatin1String("<widget class=\"Transition\" name=\"transition\">\n</widget>\n");
}

QString TransitionPlugin::includeFile() const
{
  return QLatin1String("Transition.h");
}

