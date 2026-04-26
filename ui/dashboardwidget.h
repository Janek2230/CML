#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>
#include "databasemanager.h"

namespace Ui {
class DashboardWidget;
}

class DashboardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardWidget(DatabaseManager& db, QWidget *parent = nullptr);
    ~DashboardWidget();

    void odswiezStatystykiGlowne();

signals:
    void zadaniePokazaniaSzczegolow(int idMedium);

private slots:
    void onBtnLosujClicked();

private:
    Ui::DashboardWidget *ui;
    DatabaseManager& dbManager;
};

#endif // DASHBOARDWIDGET_H