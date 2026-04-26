#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "kategoriedialog.h"
#include "platformydialog.h"
#include "statisticswidget.h"
#include "multimediaformwidget.h"

#include <qlineedit.h>
#include <utility>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QLocale>
#include <QInputDialog>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QChartView>
#include <QPieSeries>
#include <QChart>
#include <QRandomGenerator>
#include <QStackedBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QToolTip>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->btnPowrot, &QPushButton::clicked, this, [this]() {
        ui->daneSzczegolowe->setCurrentWidget(dashboardWidget);
        dashboardWidget->odswiezStatystykiGlowne();
    });

    // --- AKCJE Z MENU GŁÓWNEGO ---
    connect(ui->actionKategorie, &QAction::triggered, this, [this]() {
        KategorieDialog dialog(dbManager, this);
        dialog.exec();
        listaMultimediow = dbManager.getAllMultimedia();
        zaladujDaneDoDrzewa();
    });

    connect(ui->actionPlatformy, &QAction::triggered, this, [this]() {
        PlatformyDialog dialog(dbManager, this);
        dialog.exec();
        listaMultimediow = dbManager.getAllMultimedia();
        zaladujDaneDoDrzewa();
    });

    connect(ui->actionStronaGlowna, &QAction::triggered, this, [this]() {
            ui->panelKategorii->show();
            ui->daneSzczegolowe->setCurrentWidget(dashboardWidget);
            ui->comboWidokStatystyk->hide();
    });

    connect(ui->actionStatystyki, &QAction::triggered, this, [this]() {
        ui->panelKategorii->hide();
        // Przełączamy stos na nasz nowy widget!
        ui->daneSzczegolowe->setCurrentWidget(statsWidget);
        ui->comboWidokStatystyk->show();
        statsWidget->odswiezWykresAktywnosci();
    });

    statsWidget = new StatisticsWidget(dbManager, this);
    ui->daneSzczegolowe->addWidget(statsWidget);

    formularzWidget = new MultimediaFormWidget(dbManager, this);
    ui->daneSzczegolowe->addWidget(formularzWidget);

    // Kiedy formularz krzyknie, że zapisał dane:
    connect(formularzWidget, &MultimediaFormWidget::daneZapisane, this, [this]() {
        listaMultimediow = dbManager.getAllMultimedia();
        dashboardWidget->odswiezStatystykiGlowne();
        // Wracamy do widoku domyślnego
        ui->daneSzczegolowe->setCurrentWidget(dashboardWidget);
    });

    // Kiedy formularz krzyknie, że go anulowano:
    connect(formularzWidget, &MultimediaFormWidget::formularzAnulowany, this, [this]() {
        ui->daneSzczegolowe->setCurrentWidget(dashboardWidget);
    });

    connect(ui->btnDodajMedium, &QPushButton::clicked, this, [this]() {
        formularzWidget->przygotujFormularz(-1, 0, 0);
        ui->daneSzczegolowe->setCurrentWidget(formularzWidget);
    });

    szczegolyWidget = new SzczegolyWidget(dbManager, this);
    ui->daneSzczegolowe->addWidget(szczegolyWidget); // Wrzucamy na stos widoków

    connect(szczegolyWidget, &SzczegolyWidget::daneZaktualizowane, this, [this]() {
        listaMultimediow = dbManager.getAllMultimedia(); // Przeładowanie po zmianie
        zaladujDaneDoDrzewa();
        dashboardWidget->odswiezStatystykiGlowne();
    });

    dashboardWidget = new DashboardWidget(dbManager, this);
    ui->daneSzczegolowe->addWidget(dashboardWidget);

    // Jeśli dashboard krzyczy, że trzeba pokazać szczegóły (bo kliknięto losowanie lub kafel), MainWindow to robi
    connect(dashboardWidget, &DashboardWidget::zadaniePokazaniaSzczegolow, this, [this](int id) {
        pokazSzczegolyMedium(id);
        ui->statusbar->showMessage("Przełączono na szczegóły!", 3000); // Mały bonus dla wylosowanego
    });


    // Ukrywamy na starcie, dopóki nie wejdziemy w statystyki
    ui->comboWidokStatystyk->hide();
    // --- START APLIKACJI ---
    if (dbManager.openConnection()) {
        zaladujDaneDoDrzewa();
        dashboardWidget->odswiezStatystykiGlowne();
        ui->daneSzczegolowe->setCurrentWidget(dashboardWidget);
    } else {
        qDebug() << "Błąd połączenia z bazą danych.";
    }

}

MainWindow::~MainWindow()
{
    listaMultimediow.clear();
    delete ui;
}


void MainWindow::pokazSzczegolyMedium(int idMedium) {
    szczegolyWidget->ustawMedium(idMedium);
    ui->daneSzczegolowe->setCurrentWidget(szczegolyWidget);
}

