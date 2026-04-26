#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "kategoriedialog.h"
#include "platformydialog.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    if (!appController.inicjalizujBaze()) {
        qDebug() << "Błąd krytyczny: Baza danych nie wystartowała.";
    }

    statsWidget = new StatisticsWidget(appController, this);
    ui->daneSzczegolowe->addWidget(statsWidget);

    formularzWidget = new MultimediaFormWidget(appController.getDb(), this);
    ui->daneSzczegolowe->addWidget(formularzWidget);

    szczegolyWidget = new SzczegolyWidget(appController.getDb(), this);
    ui->daneSzczegolowe->addWidget(szczegolyWidget);

    dashboardWidget = new DashboardWidget(appController.getDb(), this);
    ui->daneSzczegolowe->addWidget(dashboardWidget);

    panelNawigacji = new PanelNawigacjiWidget(appController.getDb(), this);
    ui->splitter->insertWidget(0, panelNawigacji);

    connect(ui->actionKategorie, &QAction::triggered, this, [this]() {
        KategorieDialog dialog(appController.getDb(), this);
        dialog.exec();
        panelNawigacji->odswiezDrzewo();
        dashboardWidget->odswiezStatystykiGlowne();
    });

    connect(ui->actionPlatformy, &QAction::triggered, this, [this]() {
        PlatformyDialog dialog(appController.getDb(), this);
        dialog.exec();
        panelNawigacji->odswiezDrzewo();
        dashboardWidget->odswiezStatystykiGlowne();
    });


    connect(ui->actionStatystyki, &QAction::triggered, this, [this]() {
        panelNawigacji->hide();
        ui->daneSzczegolowe->setCurrentWidget(statsWidget);
        statsWidget->odswiezDane();
    });

    connect(ui->actionStronaGlowna, &QAction::triggered, this, [this]() {
        panelNawigacji->show();
        ui->daneSzczegolowe->setCurrentWidget(dashboardWidget);
        dashboardWidget->odswiezStatystykiGlowne();
    });


    connect(panelNawigacji, &PanelNawigacjiWidget::zadaniePokazaniaSzczegolow, this, [this](int id) {
        pokazSzczegolyMedium(id);
    });

    connect(panelNawigacji, &PanelNawigacjiWidget::zadaniePowrotuDoDashboardu, this, [this]() {
        ui->daneSzczegolowe->setCurrentWidget(dashboardWidget);
        dashboardWidget->odswiezStatystykiGlowne();
    });

    connect(panelNawigacji, &PanelNawigacjiWidget::zadanieDodaniaMedium, this, [this](int idKat, int idPlat) {
        formularzWidget->przygotujFormularz(-1, idKat, idPlat);
        ui->daneSzczegolowe->setCurrentWidget(formularzWidget);
    });

    connect(panelNawigacji, &PanelNawigacjiWidget::zadanieEdycjiMedium, this, [this](int idMedium) {
        formularzWidget->przygotujFormularz(idMedium, 0, 0);
        ui->daneSzczegolowe->setCurrentWidget(formularzWidget);
    });

    connect(panelNawigacji, &PanelNawigacjiWidget::drzewoZmieniloBaze, this, [this]() {
        dashboardWidget->odswiezStatystykiGlowne();
    });

    connect(formularzWidget, &MultimediaFormWidget::daneZapisane, this, [this]() {
        panelNawigacji->odswiezDrzewo();
        dashboardWidget->odswiezStatystykiGlowne();
        ui->daneSzczegolowe->setCurrentWidget(dashboardWidget);
    });

    connect(formularzWidget, &MultimediaFormWidget::formularzAnulowany, this, [this]() {
        ui->daneSzczegolowe->setCurrentWidget(dashboardWidget);
    });

    connect(szczegolyWidget, &SzczegolyWidget::daneZaktualizowane, this, [this]() {
        panelNawigacji->odswiezDrzewo();
        dashboardWidget->odswiezStatystykiGlowne();
    });

    connect(dashboardWidget, &DashboardWidget::zadaniePokazaniaSzczegolow, this, [this](int id) {
        pokazSzczegolyMedium(id);
        ui->statusbar->showMessage("Przełączono na szczegóły!", 3000);
    });


    panelNawigacji->odswiezDrzewo();
    dashboardWidget->odswiezStatystykiGlowne();
    ui->daneSzczegolowe->setCurrentWidget(dashboardWidget);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::pokazSzczegolyMedium(int idMedium) {
    szczegolyWidget->ustawMedium(idMedium);
    ui->daneSzczegolowe->setCurrentWidget(szczegolyWidget);
}