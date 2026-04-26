#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qtreewidget.h>
#include <QLabel>
#include <QMouseEvent>
#include "databasemanager.h"
#include "statisticswidget.h"
#include "multimediaformwidget.h"
#include "szczegolywidget.h"
#include "dashboardwidget.h"
#include "panelnawigacjiwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    Ui::MainWindow *ui;
    DatabaseManager dbManager;
    QList<std::shared_ptr<Multimedia>> listaMultimediow;

    StatisticsWidget *statsWidget;
    MultimediaFormWidget *formularzWidget;
    DashboardWidget *dashboardWidget;
    PanelNawigacjiWidget *panelNawigacji;

    bool czyTrybEdycji = false;
    int idEdytowanegoMedium = -1;
    void usunWybraneMedium(int id);
    void pokazSzczegolyMedium(int idMedium);

    SzczegolyWidget *szczegolyWidget;

};
#endif // MAINWINDOW_H
