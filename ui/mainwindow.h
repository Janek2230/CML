#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "appcontroller.h"
#include "statisticswidget.h"
#include "multimediaformwidget.h"
#include "szczegolywidget.h"
#include "dashboardwidget.h"
#include "panelnawigacjiwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    Ui::MainWindow *ui;
    AppController appController;


    StatisticsWidget *statsWidget;
    MultimediaFormWidget *formularzWidget;
    DashboardWidget *dashboardWidget;
    PanelNawigacjiWidget *panelNawigacji;
    SzczegolyWidget *szczegolyWidget;

    void pokazSzczegolyMedium(int idMedium);
};
#endif