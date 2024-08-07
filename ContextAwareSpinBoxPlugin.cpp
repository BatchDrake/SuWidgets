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
#include "ContextAwareSpinBox.h"
#include "ContextAwareSpinBoxPlugin.h"

#include <QtPlugin>

ContextAwareSpinBoxPlugin::ContextAwareSpinBoxPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void ContextAwareSpinBoxPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool ContextAwareSpinBoxPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *ContextAwareSpinBoxPlugin::createWidget(QWidget *parent)
{
  return new ContextAwareSpinBox(parent);
}

QString ContextAwareSpinBoxPlugin::name() const
{
  return QLatin1String("ContextAwareSpinBox");
}

QString ContextAwareSpinBoxPlugin::group() const
{
  return QLatin1String("");
}

QIcon ContextAwareSpinBoxPlugin::icon() const
{
  return QIcon();
}

QString ContextAwareSpinBoxPlugin::toolTip() const
{
  return QLatin1String("");
}

QString ContextAwareSpinBoxPlugin::whatsThis() const
{
  return QLatin1String("");
}

bool ContextAwareSpinBoxPlugin::isContainer() const
{
  return false;
}

QString ContextAwareSpinBoxPlugin::domXml() const
{
  return QLatin1String("<widget class=\"ContextAwareSpinBox\" name=\"contextAwareSpinBox\">\n</widget>\n");
}

QString ContextAwareSpinBoxPlugin::includeFile() const
{
  return QLatin1String("ContextAwareSpinBox.h");
}

