#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "appcontroller.h"
#include "statisticswidget.h"
#include "multimediaformwidget.h"
#include "detailswidget.h"
#include "dashboardwidget.h"
#include "navigationpanelwidget.h"
#include "timelineview.h"

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
    Ui::MainWindow      *ui;
    AppController        appController;

    TimelineView         *osCzasuWidget;
    StatisticsWidget     *statystykiWidget;
    MultimediaFormWidget *formularzWidget;
    DashboardWidget      *pulpitWidget;
    NavigationPanelWidget *panelNawigacji;
    DetailsWidget        *szczegolyWidget;

    void pokazSzczegolyMedium(int idMedium);
    void pokazPulpit();

    // Szablon dla bliźniaczych dialogów zarządzania (Kategorie/Platformy/Tagi).
    template <typename Dialog>
    void otworzDialogZarzadzania();
};
#endif
