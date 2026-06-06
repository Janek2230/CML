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
    auto statystyki = appController.pobierzStatystykiGlobalne();
    if (statystyki.value("Suma", 0) == 0) return;

    QPieSeries *seria = new QPieSeries();
    seria->append(Status::Planowane,  statystyki.value(Status::Planowane, 0));
    seria->append(Status::WTrakcie,   statystyki.value(Status::WTrakcie, 0));
    seria->append(Status::Wstrzymane, statystyki.value(Status::Wstrzymane, 0));
    seria->append(Status::Ukonczone,  statystyki.value(Status::Ukonczone, 0));
    seria->append(Status::Porzucone,  statystyki.value(Status::Porzucone, 0));

    seria->slices().at(0)->setColor(QColor(0x7f, 0x8c, 0x8d));
    seria->slices().at(1)->setColor(QColor(0x29, 0x80, 0xb9));
    seria->slices().at(2)->setColor(QColor(0xf1, 0xc4, 0x0f));
    seria->slices().at(3)->setColor(QColor(0x27, 0xae, 0x60));
    seria->slices().at(4)->setColor(QColor(0xc0, 0x39, 0x2b));

    // Aktualizacja etykiet legendy, by pokazywały konkretne wartości (np. "Planowane (15)")
    for (auto wycinek : seria->slices()) {
        wycinek->setLabel(QString("%1 (%2)").arg(wycinek->label()).arg(wycinek->value()));
    }

    QChart *wykres = new QChart();
    wykres->addSeries(seria);
    wykres->setTitle("Status Biblioteki");

    wykres->legend()->setVisible(true);
    wykres->legend()->setAlignment(Qt::AlignRight);
    wykres->setAnimationOptions(QChart::SeriesAnimations);
    wykres->setBackgroundVisible(false);
    wykres->setTitleBrush(QBrush(Qt::white));
    wykres->legend()->setLabelColor(Qt::white);

    ui->wykresOwalny->setChart(wykres);

    // Czyszczenie układu siatki ze starych kafelków przed wygenerowaniem nowych
    QLayoutItem *element;
    while ((element = ui->gridOstatnie->takeAt(0)) != nullptr) {
        delete element->widget();
        delete element;
    }

    const QList<int> ostatnieId = appController.pobierzOstatnioAktywne(6);
    int wiersz = 0, kolumna = 0;
    const auto lista = appController.pobierzWszystkieMultimedia();

    for (int id : ostatnieId) {
        for (const auto& m : lista) {
            if (m->id == id) {
                QPushButton *btnKafel = new QPushButton(m->tytul, this);

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
