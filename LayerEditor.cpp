#include "LayerEditor.h"
#include "LayerEditorModel.h"

#include "ui_LayerEditor.h"

void
LayerEditor::connectAll(void)
{
  connect(
        this->ui->addButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onAdd(void)));

  connect(
        this->ui->removeButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onRemove(void)));

  connect(
        this->ui->moveUpButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onMoveUp(void)));

  connect(
        this->ui->moveDownButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onMoveDown(void)));
}


LayerEditor::LayerEditor(QWidget *parent) :
  QWidget(parent),
  model(new LayerEditorModel(this)),
  ui(new Ui::LayerEditor)
{
  ui->setupUi(this);

  ui->layerView->setModel(this->model);
}

void
LayerEditor::add(LayerItem const &item)
{
  int index = this->ui->layerView->currentIndex().row();

  if (index == -1)
    index = 0;

  this->model->insertBefore(item, index);
}

void
LayerEditor::remove(int index)
{
  this->model->remove(index);
}

int
LayerEditor::size(void) const
{
  return this->model->rowCount();
}

LayerItem const &
LayerEditor::get(int index) const
{
  return this->model->get(index);
}

LayerEditor::~LayerEditor()
{
  delete ui;
  delete model;
}


/////////////////////////////////////// Slots /////////////////////////////////
void
LayerEditor::onAdd(void)
{
  emit addEntry();
}

void
LayerEditor::onRemove(void)
{
  int index = this->ui->layerView->currentIndex().row();

  if (index != -1) {
    emit removeEntry(index);
    this->model->remove(index);
  }
}

void
LayerEditor::onMoveUp(void)
{
  int index = this->ui->layerView->currentIndex().row();

  if (index != -1 && index > 0) {
    int dest = index - 1;
    this->model->swap(index, dest);
    emit reorderEntry(index, dest);
  }
}

void
LayerEditor::onMoveDown(void)
{
  int index = this->ui->layerView->currentIndex().row();

  if (index != -1 && index < this->model->rowCount() - 1) {
    int dest = index + 1;
    this->model->swap(index, dest);
    emit reorderEntry(index, dest);
  }
}
