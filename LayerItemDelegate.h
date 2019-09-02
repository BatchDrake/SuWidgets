#ifndef LAYERITEMDELEGATE_H
#define LAYERITEMDELEGATE_H

#include <QStyledItemDelegate>

class LayerItemDelegate : public QStyledItemDelegate
{
  Q_OBJECT

  inline QFont nameFont(const QStyleOptionViewItem &option) const;
  inline QFont descriptionFont(const QStyleOptionViewItem &option) const;

  inline QRect nameBox(
      const QStyleOptionViewItem &option,
      const QModelIndex &index) const;
  inline QRect descriptionBox(
      const QStyleOptionViewItem &option,
      const QModelIndex &index) const;

  QSize myIconSize;
  QMargins margins;
  int spacingHorizontal;
  int spacingVertical;

public:
  LayerItemDelegate(QObject *parent = nullptr);

  QSize iconSize() const;
  void setIconSize(int width, int height);

  QMargins contentsMargins() const;
  void setContentsMargins(int left, int top, int right, int bottom);

  int horizontalSpacing() const;
  void setHorizontalSpacing(int spacing);

  int verticalSpacing() const;
  void setVerticalSpacing(int spacing);

  void paint(
      QPainter *painter,
      const QStyleOptionViewItem &option,
      const QModelIndex &index) const override;

  QSize sizeHint(
      const QStyleOptionViewItem &option,
      const QModelIndex &index) const override;

};

#endif // LAYERITEMDELEGATE_H
