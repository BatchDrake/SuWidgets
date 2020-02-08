//
//    QVerticalLabelPlugin.cpp: Plugin for vertically oriented label
//    Copyright (C) 2020 Gonzalo José Carracedo Carballal
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
#include "QVerticalLabel.h"
#include "QVerticalLabelPlugin.h"

#include <QtPlugin>

QVerticalLabelPlugin::QVerticalLabelPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void QVerticalLabelPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool QVerticalLabelPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *QVerticalLabelPlugin::createWidget(QWidget *parent)
{
  return new QVerticalLabel(parent);
}

QString QVerticalLabelPlugin::name() const
{
  return QLatin1String("QVerticalLabel");
}

QString QVerticalLabelPlugin::group() const
{
  return QLatin1String("");
}

QIcon QVerticalLabelPlugin::icon() const
{
  return QIcon();
}

QString QVerticalLabelPlugin::toolTip() const
{
  return QLatin1String("");
}

QString QVerticalLabelPlugin::whatsThis() const
{
  return QLatin1String("");
}

bool QVerticalLabelPlugin::isContainer() const
{
  return false;
}

QString QVerticalLabelPlugin::domXml() const
{
  return QLatin1String("<widget class=\"QVerticalLabel\" name=\"verticalLabel\">\n</widget>\n");
}

QString QVerticalLabelPlugin::includeFile() const
{
  return QLatin1String("QVerticalLabel.h");
}

