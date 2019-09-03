#ifndef LAYEREDITOR_H
#define LAYEREDITOR_H

#include <QWidget>

namespace Ui {
  class LayerEditor;
}

class LayerEditorModel;
class LayerItemDelegate;
class LayerItem;

class LayerEditor : public QWidget
{
  Q_OBJECT

  LayerEditorModel *model = nullptr;
  LayerItemDelegate *delegate = nullptr;
  void connectAll(void);

public:
  LayerEditor(QWidget *parent = nullptr);
  ~LayerEditor();

  void add(LayerItem const &);
  int size(void) const;
  LayerItem const &get(int index) const;
  void remove(int index);

public slots:
  void onAdd(void);
  void onRemove(void);
  void onMoveUp(void);
  void onMoveDown(void);

signals:
  void addEntry(void);
  void removeEntry(int);
  void reorderEntry(int, int);

private:
  Ui::LayerEditor *ui;
};

#endif // LAYEREDITOR_H
