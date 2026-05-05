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

    series->slices().at(0)->setColor(QColor("#7f8c8d"));
    series->slices().at(1)->setColor(QColor("#2980b9"));
    series->slices().at(2)->setColor(QColor("#f1c40f")); // Wstrzymane
    series->slices().at(3)->setColor(QColor("#27ae60"));
    series->slices().at(4)->setColor(QColor("#c0392b"));

    double suma = stats.value("Suma", 0);

    for(auto slice : series->slices()) {
        if (slice->value() > 0) {
            slice->setLabel(QString("%1: %2").arg(slice->label()).arg(slice->value()));

            double procent = (slice->value() / suma) * 100.0;
        }
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
    ui->wykresOwalny->setRenderHint(QPainter::Antialiasing);

    ui->wykresOwalny->setBackgroundBrush(Qt::NoBrush);
    ui->wykresOwalny->setStyleSheet("background: transparent; border: none;");

    QLayoutItem *child;
    while ((child = ui->gridOstatnie->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    QList<int> ostatnieId = appController.pobierzOstatnioAktywne(6);
    int wiersz = 0, kolumna = 0;

    auto lista = appController.pobierzWszystkieMultimedia();

    for (int id : ostatnieId) {
        for (const auto& m : lista) {
            if (m->getId() == id) {
                QPushButton *btnKafel = new QPushButton(m->getTytul(), this);
                btnKafel->setMinimumHeight(50);
                connect(btnKafel, &QPushButton::clicked, this, [this, id]() { emit zadaniePokazaniaSzczegolow(id); });
                ui->gridOstatnie->addWidget(btnKafel, wiersz, kolumna);
                kolumna++;
                if (kolumna > 1) { kolumna = 0; wiersz++; }
                break;
            }
        }
    }
}

