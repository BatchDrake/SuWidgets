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
#include "Waveform.h"
#include "WaveformPlugin.h"

#include <QtPlugin>

WaveformPlugin::WaveformPlugin(QObject *parent)
  : QObject(parent)
{
  m_initialized = false;
}

void WaveformPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
  if (m_initialized)
    return;

  // Add extension registrations, etc. here

  m_initialized = true;
}

bool WaveformPlugin::isInitialized() const
{
  return m_initialized;
}

QWidget *WaveformPlugin::createWidget(QWidget *parent)
{
  return new Waveform(parent);
}

QString WaveformPlugin::name() const
{
  return QLatin1String("Waveform");
}

QString WaveformPlugin::group() const
{
  return QLatin1String("");
}

QIcon WaveformPlugin::icon() const
{
  return QIcon();
}

QString WaveformPlugin::toolTip() const
{
  return QLatin1String("");
}

QString WaveformPlugin::whatsThis() const
{
  return QLatin1String("");
}

bool WaveformPlugin::isContainer() const
{
  return false;
}

QString WaveformPlugin::domXml() const
{
  return QLatin1String("<widget class=\"Waveform\" name=\"waveform\">\n</widget>\n");
}

QString WaveformPlugin::includeFile() const
{
  return QLatin1String("Waveform.h");
}

