#ifndef LAYEREDITORMODEL_H
#define LAYEREDITORMODEL_H

#include <QAbstractListModel>
#include <LayerItem.h>
#include <vector>


class LayerEditorModel : public QAbstractListModel
{
  QList<LayerItem> items;

public:
  LayerEditorModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  LayerItem const &get(int index) const;
  void insertBefore(LayerItem const &item, int index = -1);
  void insertAfter(LayerItem const &item, int index = -1);
  void swap(int a, int b);
  void remove(int index);
};

#endif // LAYEREDITORMODEL_H

