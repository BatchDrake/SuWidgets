#include "LayerItem.h"

#include "LayerEditorModel.h"

static void
LayerEditorAssertTypes(void)
{
  static bool registered = false;

  if (!registered) {
    qRegisterMetaType<LayerItem>();
    registered = true;
  }
}

LayerEditorModel::LayerEditorModel(QObject *parent) : QAbstractListModel (parent)
{
  LayerEditorAssertTypes();
}

int
LayerEditorModel::rowCount(const QModelIndex &) const
{
  return this->items.size();
}

QVariant
LayerEditorModel::data(const QModelIndex &index, int role) const
{
  if (role == Qt::DisplayRole) {
    LayerItem const &item = this->items[index.row()];
    return QVariant::fromValue(item);
  } else {
    return QVariant();
  }
}

LayerItem const &
LayerEditorModel::get(int index) const
{
  return this->items[index];
}

void
LayerEditorModel::insertBefore(LayerItem const &item, int index)
{
  this->beginInsertRows(QModelIndex(), index, index);
  this->items.insert(index, item);
  this->endInsertRows();
}

void
LayerEditorModel::insertAfter(LayerItem const &item, int index)
{
  this->insertBefore(item, index + 1);
}

void
LayerEditorModel::swap(int a, int b)
{
  this->beginMoveRows(QModelIndex(), a, a, QModelIndex(), b);
  this->items.swap(a, b);
  this->endMoveRows();
}

void
LayerEditorModel::remove(int index)
{
  this->beginRemoveRows(QModelIndex(), index, index);
  this->items.takeAt(index);
}
