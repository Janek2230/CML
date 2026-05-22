#include "dashboardwidget.h"
#include "ui_dashboardwidget.h"

#include <QChart>
#include <QPieSeries>
#include <QChartView>
#include <QPushButton>
#include <QLayoutItem>
#include <QColor>

DashboardWidget::DashboardWidget(AppController& controller, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DashboardWidget),
    appController(controller)
{
    ui->setupUi(this);
}

DashboardWidget::~DashboardWidget() {
    delete ui;
}

void DashboardWidget::odswiezStatystykiGlowne() {
    auto stats = appController.getGlobalStats();
    if (stats.value("Suma", 0) == 0) return;

    QPieSeries *series = new QPieSeries();
    series->append("Planowane", stats.value("Planowane", 0));
    series->append("W trakcie", stats.value("W trakcie", 0));
    series->append("Wstrzymane", stats.value("Wstrzymane", 0));
    series->append("Ukończone", stats.value("Ukończone", 0));
    series->append("Porzucone", stats.value("Porzucone", 0));

    series->slices().at(0)->setColor(QColor(0x7f, 0x8c, 0x8d));
    series->slices().at(1)->setColor(QColor(0x29, 0x80, 0xb9));
    series->slices().at(2)->setColor(QColor(0xf1, 0xc4, 0x0f));
    series->slices().at(3)->setColor(QColor(0x27, 0xae, 0x60));
    series->slices().at(4)->setColor(QColor(0xc0, 0x39, 0x2b));

    // Aktualizacja etykiet legendy, by pokazywały konkretne wartości (np. "Planowane (15)")
    for (auto slice : series->slices()) {
        slice->setLabel(QString("%1 (%2)").arg(slice->label()).arg(slice->value()));
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Status Biblioteki");

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundVisible(false);
    chart->setTitleBrush(QBrush(Qt::white));
    chart->legend()->setLabelColor(Qt::white);

    ui->wykresOwalny->setChart(chart);

    // Czyszczenie układu siatki ze starych kafelków przed wygenerowaniem nowych
    QLayoutItem *child;
    while ((child = ui->gridOstatnie->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    const QList<int> ostatnieId = appController.pobierzOstatnioAktywne(6);
    int wiersz = 0, kolumna = 0;
    const auto lista = appController.pobierzWszystkieMultimedia();

    for (int id : ostatnieId) {
        for (const auto& m : lista) {
            if (m->getId() == id) {
                QPushButton *btnKafel = new QPushButton(m->getTytul(), this);

                connect(btnKafel, &QPushButton::clicked, this, [this, id]() {
                    emit zadaniePokazaniaSzczegolow(id);
                });

                ui->gridOstatnie->addWidget(btnKafel, wiersz, kolumna);

                // Logika łamania wierszy (maks. 2 elementy w rzędzie)
                kolumna++;
                if (kolumna > 1) {
                    kolumna = 0;
                    wiersz++;
                }
                break;
            }
        }
    }
}
