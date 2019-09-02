#ifndef LAYERITEM_H
#define LAYERITEM_H

#include <QObject>
#include <QMetaType>
#include <QIcon>
#include <QVariant>

class LayerItem
{
  QIcon layerIcon;
  QString layerName;
  QString layerDescription;
  QVariant layerData;

public:
  LayerItem();
  LayerItem(LayerItem const &prev);
  ~LayerItem();

  QIcon icon(void) const;
  QString name(void) const;
  QString description(void) const;
  QVariant data(void) const;

  void setIcon(QIcon const &);
  void setName(QString const &);
  void setDescription(QString const &);
  void setData(QVariant const &);
};

Q_DECLARE_METATYPE(LayerItem);

#endif // LAYERITEM_H
