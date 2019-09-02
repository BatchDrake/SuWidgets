#include "LayerItem.h"

LayerItem::LayerItem(void)
{

}

LayerItem::LayerItem(LayerItem const &prev)
{
  this->layerIcon = prev.layerIcon;
  this->layerName = prev.layerName;
  this->layerDescription = prev.layerDescription;
  this->layerData = prev.layerData;
}

LayerItem::~LayerItem(void)
{

}

QIcon
LayerItem::icon(void) const
{
  return this->layerIcon;
}

QString
LayerItem::name(void) const
{
  return this->layerName;
}

QString
LayerItem::description(void) const
{
  return this->layerDescription;
}

QVariant
LayerItem::data(void) const
{
  return this->layerData;
}

void
LayerItem::setIcon(QIcon const &icon)
{
  this->layerIcon = icon;
}

void
LayerItem::setDescription(QString const &desc)
{
  this->layerDescription = desc;
}

void
LayerItem::setName(QString const &name)
{
  this->layerName = name;
}

void
LayerItem::setData(QVariant const &data)
{
  this->layerData = data;
}
