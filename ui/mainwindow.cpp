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

    // Widoki
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

    // Połączenia sygnałów
    connect(ui->actionKategorie, &QAction::triggered, this, [this]() { otworzDialogZarzadzania<CategoriesDialog>(); });
    connect(ui->actionPlatformy, &QAction::triggered, this, [this]() { otworzDialogZarzadzania<PlatformsDialog>(); });
    connect(ui->actionTagi,      &QAction::triggered, this, [this]() { otworzDialogZarzadzania<TagsDialog>(); });

    connect(ui->actionStronaGlowna, &QAction::triggered, this, [this]() {
        panelNawigacji->show();
        pokazPulpit();
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
        pokazPulpit();
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
        pokazPulpit();
    });

    connect(formularzWidget, &MultimediaFormWidget::formularzAnulowany, this, [this]() {
        pokazPulpit();
    });

    connect(szczegolyWidget, &DetailsWidget::daneZaktualizowane, this, [this]() {
        pulpitWidget->odswiezStatystykiGlowne();
    });

    connect(pulpitWidget, &DashboardWidget::zadaniePokazaniaSzczegolow, this, [this](int id) {
        pokazSzczegolyMedium(id);
    });

    connect(statystykiWidget, &StatisticsWidget::zadaniePokazaniaSzczegolow, this, [this](int id) {
        pokazSzczegolyMedium(id);
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


    // Widok startowy
    panelNawigacji->odswiezDrzewo();
    pokazPulpit();
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

void MainWindow::pokazPulpit() {
    ui->daneSzczegolowe->setCurrentWidget(pulpitWidget);
    pulpitWidget->odswiezStatystykiGlowne();
}

// Otwiera modalny dialog zarządzania
template <typename Dialog>
void MainWindow::otworzDialogZarzadzania() {
    Dialog dialog(appController, this);
    dialog.exec();
    pulpitWidget->odswiezStatystykiGlowne();
}
