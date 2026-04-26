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

    statsWidget = new StatisticsWidget(dbManager, this);
    ui->daneSzczegolowe->addWidget(statsWidget);

    formularzWidget = new MultimediaFormWidget(dbManager, this);
    ui->daneSzczegolowe->addWidget(formularzWidget);

    szczegolyWidget = new SzczegolyWidget(dbManager, this);
    ui->daneSzczegolowe->addWidget(szczegolyWidget);

    dashboardWidget = new DashboardWidget(dbManager, this);
    ui->daneSzczegolowe->addWidget(dashboardWidget);

    panelNawigacji = new PanelNawigacjiWidget(dbManager, this);
    ui->splitter->insertWidget(0, panelNawigacji);
    ui->splitter->setStretchFactor(0, 0);
    ui->splitter->setStretchFactor(1, 1);

    connect(ui->actionKategorie, &QAction::triggered, this, [this]() {
        KategorieDialog dialog(dbManager, this);
        dialog.exec();
        panelNawigacji->odswiezDrzewo();
        dashboardWidget->odswiezStatystykiGlowne();
    });

    connect(ui->actionPlatformy, &QAction::triggered, this, [this]() {
        PlatformyDialog dialog(dbManager, this);
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


    if (dbManager.openConnection()) {
        panelNawigacji->odswiezDrzewo();
        dashboardWidget->odswiezStatystykiGlowne();
        ui->daneSzczegolowe->setCurrentWidget(dashboardWidget);
    } else {
        qDebug() << "Błąd połączenia z bazą danych.";
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::pokazSzczegolyMedium(int idMedium) {
    szczegolyWidget->ustawMedium(idMedium);
    ui->daneSzczegolowe->setCurrentWidget(szczegolyWidget);
}