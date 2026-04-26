#ifndef STATISTICSWIDGET_H
#define STATISTICSWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QEvent>
#include <QMouseEvent>
#include "appcontroller.h"

namespace Ui {
class StatisticsWidget;
}

class StatisticsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StatisticsWidget(AppController& controller, QWidget *parent = nullptr);
    ~StatisticsWidget();

    void odswiezWykresAktywnosci();
    void odswiezDane();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::StatisticsWidget *ui;
    AppController& appController;
    QLabel *etykietaTooltip;

private slots:
    void onComboWidokChanged(int index);
};



#endif