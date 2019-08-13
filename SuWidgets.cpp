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
#include "ConstellationPlugin.h"
#include "TransitionPlugin.h"
#include "HistogramPlugin.h"
#include "LCDPlugin.h"
#include "WaveformPlugin.h"
#include "SymViewPlugin.h"
#include "SuWidgets.h"

SuWidgets::SuWidgets(QObject *parent)
  : QObject(parent)
{
  m_widgets.append(new ConstellationPlugin(this));
  m_widgets.append(new TransitionPlugin(this));
  m_widgets.append(new HistogramPlugin(this));
  m_widgets.append(new LCDPlugin(this));
  m_widgets.append(new WaveformPlugin(this));
  m_widgets.append(new SymViewPlugin(this));

}

QList<QDesignerCustomWidgetInterface*> SuWidgets::customWidgets() const
{
  return m_widgets;
}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(suwidgetsplugin, SuWidgets)
#endif // QT_VERSION < 0x050000
