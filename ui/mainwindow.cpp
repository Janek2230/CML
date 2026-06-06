#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "categoriesdialog.h"
#include "platformsdialog.h"
#include "tagsdialog.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    if (!appController.inicjalizujBaze()) {
        QMessageBox::critical(this, "Błąd krytyczny",
            "Nie udało się połączyć z bazą danych.\nSprawdź plik config.ini.");
    }

    osCzasuWidget = new TimelineView(appController, this);
    ui->daneSzczegolowe->addWidget(osCzasuWidget);

    statystykiWidget = new StatisticsWidget(appController, this);
    ui->daneSzczegolowe->addWidget(statystykiWidget);

    formularzWidget = new MultimediaFormWidget(appController, this);
    ui->daneSzczegolowe->addWidget(formularzWidget);

    szczegolyWidget = new DetailsWidget(appController, this);
    ui->daneSzczegolowe->addWidget(szczegolyWidget);

    pulpitWidget = new DashboardWidget(appController, this);
    ui->daneSzczegolowe->addWidget(pulpitWidget);

    panelNawigacji = new NavigationPanelWidget(appController, this);
    ui->splitter->insertWidget(0, panelNawigacji);

    connect(ui->actionKategorie, &QAction::triggered, this, [this]() {
        CategoriesDialog dialog(appController, this);
        dialog.exec();
        pulpitWidget->odswiezStatystykiGlowne();
    });

    connect(ui->actionPlatformy, &QAction::triggered, this, [this]() {
        PlatformsDialog dialog(appController, this);
        dialog.exec();
        pulpitWidget->odswiezStatystykiGlowne();
    });

    connect(ui->actionTagi, &QAction::triggered, this, [this]() {
        TagsDialog dialog(appController, this);
        dialog.exec();
        pulpitWidget->odswiezStatystykiGlowne();
    });

    connect(ui->actionStronaGlowna, &QAction::triggered, this, [this]() {
        panelNawigacji->show();
        ui->daneSzczegolowe->setCurrentWidget(pulpitWidget);
        pulpitWidget->odswiezStatystykiGlowne();
    });

    connect(ui->actionStatystyki, &QAction::triggered, this, [this]() {
        panelNawigacji->hide();
        ui->daneSzczegolowe->setCurrentWidget(statystykiWidget);
        statystykiWidget->odswiezDane();
    });

    connect(panelNawigacji, &NavigationPanelWidget::zadaniePokazaniaSzczegolow, this, [this](int id) {
        pokazSzczegolyMedium(id);
    });

    connect(panelNawigacji, &NavigationPanelWidget::zadaniePowrotuDoDashboardu, this, [this]() {
        ui->daneSzczegolowe->setCurrentWidget(pulpitWidget);
        pulpitWidget->odswiezStatystykiGlowne();
    });

    connect(panelNawigacji, &NavigationPanelWidget::zadanieDodaniaMedium, this, [this](int idKat, int idPlat) {
        formularzWidget->przygotujFormularz(-1, idKat, idPlat);
        ui->daneSzczegolowe->setCurrentWidget(formularzWidget);
    });

    connect(panelNawigacji, &NavigationPanelWidget::zadanieEdycjiMedium, this, [this](int idMedium) {
        formularzWidget->przygotujFormularz(idMedium, 0, 0);
        ui->daneSzczegolowe->setCurrentWidget(formularzWidget);
    });

    connect(formularzWidget, &MultimediaFormWidget::daneZapisane, this, [this]() {
        pulpitWidget->odswiezStatystykiGlowne();
        ui->daneSzczegolowe->setCurrentWidget(pulpitWidget);
    });

    connect(formularzWidget, &MultimediaFormWidget::formularzAnulowany, this, [this]() {
        ui->daneSzczegolowe->setCurrentWidget(pulpitWidget);
    });

    connect(szczegolyWidget, &DetailsWidget::daneZaktualizowane, this, [this]() {
        pulpitWidget->odswiezStatystykiGlowne();
    });

    connect(pulpitWidget, &DashboardWidget::zadaniePokazaniaSzczegolow, this, [this](int id) {
        pokazSzczegolyMedium(id);
        ui->statusbar->showMessage("Przełączono na szczegóły!", 3000);
    });

    connect(statystykiWidget, &StatisticsWidget::zadaniePokazaniaSzczegolow, this, [this](int id) {
        pokazSzczegolyMedium(id);
        ui->statusbar->showMessage("Otworzono propozycję z Kupki Wstydu.", 3000);
    });

    connect(&appController, &AppController::daneZmienione, this, [this]() {
        panelNawigacji->odswiezDrzewo();
        pulpitWidget->odswiezStatystykiGlowne();
    });

    connect(ui->actionMojeRecenzje, &QAction::triggered, this, [this]() {
        panelNawigacji->hide(); // Chowamy panel nawigacji dla pełnoekranowego efektu osi czasu.
        ui->daneSzczegolowe->setCurrentWidget(osCzasuWidget);
        osCzasuWidget->renderujOsCzasu();
    });


    panelNawigacji->odswiezDrzewo();
    pulpitWidget->odswiezStatystykiGlowne();
    ui->daneSzczegolowe->setCurrentWidget(pulpitWidget);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::pokazSzczegolyMedium(int idMedium) {
    panelNawigacji->show();
    szczegolyWidget->ustawMedium(idMedium);
    ui->daneSzczegolowe->setCurrentWidget(szczegolyWidget);
}
