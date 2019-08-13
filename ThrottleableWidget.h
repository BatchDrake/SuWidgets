#ifndef THROTTLEABLEWIDGET_H
#define THROTTLEABLEWIDGET_H

#include <QWidget>

class ThrottleableWidget : public QWidget
{
    Q_OBJECT
  public:
    explicit ThrottleableWidget(QWidget *parent = nullptr);

  signals:

  public slots:
};

#endif // THROTTLEABLEWIDGET_H