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
#include "LayerEditor.h"
#include "LayerEditorPlugin.h"

#include <QtPlugin>

LayerEditorPlugin::LayerEditorPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void LayerEditorPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool LayerEditorPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *LayerEditorPlugin::createWidget(QWidget *parent)
{
  return new LayerEditor(parent);
}

QString LayerEditorPlugin::name() const
{
  return QLatin1String("LayerEditor");
}

QString LayerEditorPlugin::group() const
{
  return QLatin1String("");
}

QIcon LayerEditorPlugin::icon() const
{
  return QIcon();
}

QString LayerEditorPlugin::toolTip() const
{
  return QLatin1String("");
}

QString LayerEditorPlugin::whatsThis() const
{
  return QLatin1String("");
}

bool LayerEditorPlugin::isContainer() const
{
  return false;
}

QString LayerEditorPlugin::domXml() const
{
  return QLatin1String("<widget class=\"LayerEditor\" name=\"LayerEditor\">\n</widget>\n");
}

QString LayerEditorPlugin::includeFile() const
{
  return QLatin1String("LayerEditor.h");
}

