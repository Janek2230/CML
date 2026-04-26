#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>
#include "appcontroller.h"

namespace Ui {
class DashboardWidget;
}

class DashboardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardWidget(AppController& controller, QWidget *parent = nullptr);
    ~DashboardWidget();

    void odswiezStatystykiGlowne();

signals:
    void zadaniePokazaniaSzczegolow(int idMedium);

private slots:
    void onBtnLosujClicked();

private:
    Ui::DashboardWidget *ui;
    AppController& appController;
};

#endif // DASHBOARDWIDGET_H
