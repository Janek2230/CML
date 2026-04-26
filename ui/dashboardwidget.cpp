#include "dashboardwidget.h"
#include "ui_dashboardwidget.h"
#include <QMessageBox>
#include <QPieSeries>
#include <QChart>
#include <QPushButton>
#include <QRandomGenerator>

DashboardWidget::DashboardWidget(DatabaseManager& db, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DashboardWidget),
    dbManager(db)
{
    ui->setupUi(this);
    connect(ui->btnLosuj, &QPushButton::clicked, this, &DashboardWidget::onBtnLosujClicked);
}

DashboardWidget::~DashboardWidget()
{
    delete ui;
}

void DashboardWidget::odswiezStatystykiGlowne() {
    auto stats = dbManager.getGlobalStats();
    if (stats.value("Suma", 0) == 0) return;

    QPieSeries *series = new QPieSeries();
    series->append("Planowane", stats.value("Planowane", 0));
    series->append("W trakcie", stats.value("W trakcie", 0));
    series->append("Ukończone", stats.value("Ukończone", 0));
    series->append("Porzucone", stats.value("Porzucone", 0));

    series->slices().at(0)->setColor(QColor("#7f8c8d"));
    series->slices().at(1)->setColor(QColor("#2980b9"));
    series->slices().at(2)->setColor(QColor("#27ae60"));
    series->slices().at(3)->setColor(QColor("#c0392b"));

    for(auto slice : series->slices()) {
        if (slice->value() > 0) {
            slice->setLabelVisible(true);
            slice->setLabel(QString("%1: %2").arg(slice->label()).arg(slice->value()));
        }
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Status Biblioteki");
    chart->legend()->hide();
    chart->setAnimationOptions(QChart::SeriesAnimations);

    ui->wykresOwalny->setChart(chart);
    ui->wykresOwalny->setRenderHint(QPainter::Antialiasing);

    QLayoutItem *child;
    while ((child = ui->gridOstatnie->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    QList<int> ostatnieId = dbManager.pobierzOstatnioAktywne(6);
    int wiersz = 0, kolumna = 0;

    auto lista = dbManager.getAllMultimedia(); // Brute-force pobranie tytułów

    for (int id : ostatnieId) {
        for (const auto& m : lista) {
            if (m->getId() == id) {
                QPushButton *btnKafel = new QPushButton(m->getTytul(), this);
                btnKafel->setMinimumHeight(50);
                // Emitujemy sygnał, zamiast wołać bezposrednio pokazSzczegolyMedium
                connect(btnKafel, &QPushButton::clicked, this, [this, id]() { emit zadaniePokazaniaSzczegolow(id); });
                ui->gridOstatnie->addWidget(btnKafel, wiersz, kolumna);
                kolumna++;
                if (kolumna > 1) { kolumna = 0; wiersz++; }
                break;
            }
        }
    }
}

void DashboardWidget::onBtnLosujClicked() {
    auto lista = dbManager.getAllMultimedia();
    QList<int> doWylosowania;
    for (const auto& m : lista) {
        if (m->getStatus() == "Planowane") doWylosowania.append(m->getId());
    }
    if (doWylosowania.isEmpty()) {
        QMessageBox::information(this, "Pusto!", "Nie masz żadnych 'Planowanych' pozycji.");
        return;
    }
    int wylosowaneId = doWylosowania.at(QRandomGenerator::global()->bounded(doWylosowania.size()));

    // Zamiast zmieniać widok, emitujemy sygnał, że wylosowano grę
    emit zadaniePokazaniaSzczegolow(wylosowaneId);
}