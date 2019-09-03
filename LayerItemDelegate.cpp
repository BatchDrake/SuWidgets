#include "LayerItemDelegate.h"
#include "LayerItem.h"

#include <QPainter>

QFont
LayerItemDelegate::nameFont(const QStyleOptionViewItem &option) const
{
  QFont f(option.font);

  f.setBold(true);

  return f;
}

QFont
LayerItemDelegate::descriptionFont(const QStyleOptionViewItem &option) const
{
  QFont f(option.font);

  f.setPointSizeF(0.85 * option.font.pointSizeF());

  return f;
}


QRect
LayerItemDelegate::nameBox(
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
  return QFontMetrics(this->nameFont(option)).boundingRect(
        index.data(Qt::DisplayRole).value<LayerItem>().name()).adjusted(0, 0, 1, 1);
}


QRect
LayerItemDelegate::descriptionBox(
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
  return QFontMetrics(this->descriptionFont(option)).boundingRect(
        index.data(Qt::DisplayRole).value<LayerItem>().description()).adjusted(0, 0, 1, 1);
}

QSize
LayerItemDelegate::sizeHint(
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
  QStyleOptionViewItem opt(option);
  initStyleOption(&opt, index);

  int textHeight = this->nameBox(option, index).height()
                  + this->spacingVertical + this->descriptionBox(option, index).height();
  int iconHeight = this->myIconSize.height();
  int h = textHeight > iconHeight ? textHeight : iconHeight;

  return QSize(opt.rect.width(), this->margins.top() + h + this->margins.bottom());
}

void
LayerItemDelegate::paint(
    QPainter *painter,
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
  QStyleOptionViewItem opt(option);
  initStyleOption(&opt, index);

  const QPalette &palette(opt.palette);
  const QRect &rect(opt.rect);
  const QRect &contentRect(
        rect.adjusted(
          this->margins.left(),
          this->margins.top(),
          -this->margins.right(),
          -this->margins.bottom()));
  const bool lastIndex = (index.model()->rowCount() - 1) == index.row();
  const int bottomEdge = rect.bottom();

  painter->save();
  painter->setClipping(true);
  painter->setClipRect(rect);
  painter->setFont(opt.font);

  // Draw background
  painter->fillRect(
        rect,
        opt.state & QStyle::State_Selected
          ? palette.highlight().color()
          : palette.light().color());

  // Draw bottom line
  painter->setPen(
        lastIndex
          ? palette.dark().color()
          : palette.mid().color());

  painter->drawLine(
        lastIndex
          ? rect.left()
          : this->margins.left(),
        bottomEdge,
        rect.right(),
        bottomEdge);

  // Draw message icon
  if (!index.data(Qt::DisplayRole).value<LayerItem>().icon().isNull())
    painter->drawPixmap(
          contentRect.left(),
          contentRect.top(),
          index.data(Qt::DisplayRole).value<LayerItem>().icon().pixmap(this->myIconSize));

  // Draw name
  QRect nameRect(this->nameBox(opt, index));

  nameRect.moveTo(
        this->margins.left()
        + this->myIconSize.width()
        + this->spacingHorizontal,
        contentRect.top());

  painter->setFont(this->nameFont(opt));
  painter->setPen(opt.state & QStyle::State_Selected ? palette.highlightedText().color() : palette.text().color());
  painter->drawText(
        nameRect,
        Qt::TextSingleLine,
        index.data(Qt::DisplayRole).value<LayerItem>().name());

  // Draw description
  QRect descriptionRect(this->descriptionBox(opt, index));

  descriptionRect.moveTo(
        nameRect.left(),
        nameRect.bottom() + this->spacingVertical);

  painter->setFont(this->descriptionFont(opt));
  painter->setPen(opt.state & QStyle::State_Selected ? palette.highlightedText().color() : palette.text().color());
  painter->drawText(
        descriptionRect,
        Qt::TextSingleLine,
        index.data(Qt::DisplayRole).value<LayerItem>().description());

  painter->restore();
}

QSize
LayerItemDelegate::iconSize() const
{
  return this->myIconSize;
}

void
LayerItemDelegate::setIconSize(int width, int height)
{
  this->myIconSize = QSize(width, height);
}

QMargins
LayerItemDelegate::contentsMargins() const
{
  return this->margins;
}

void
LayerItemDelegate::setContentsMargins(int left, int top, int right, int bottom)
{
  this->margins = QMargins(left, top, right, bottom);
}

int
LayerItemDelegate::horizontalSpacing() const
{
  return this->spacingHorizontal;
}

void
LayerItemDelegate::setHorizontalSpacing(int spacing)
{
  this->spacingHorizontal = spacing;
}

int
LayerItemDelegate::verticalSpacing() const
{
  return this->spacingVertical;
}

void
LayerItemDelegate::setVerticalSpacing(int spacing)
{
  this->spacingVertical = spacing;
}

LayerItemDelegate::LayerItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{

}

