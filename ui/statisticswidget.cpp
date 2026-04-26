#include "statisticswidget.h"
#include "ui_statisticswidget.h"

#include <QChartView>
#include <QStackedBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>

StatisticsWidget::StatisticsWidget(DatabaseManager& db, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StatisticsWidget),
    appController(controller)
{
    ui->setupUi(this);

    ui->wykresSlupkowyAktywnosci->setMouseTracking(true);
    ui->wykresSlupkowyAktywnosci->viewport()->setMouseTracking(true);
    ui->wykresSlupkowyAktywnosci->viewport()->installEventFilter(this);

    etykietaTooltip = new QLabel(ui->wykresSlupkowyAktywnosci->viewport());
    etykietaTooltip->setStyleSheet("background-color: rgba(30, 30, 30, 220); color: white; border: 1px solid #555; border-radius: 4px; padding: 5px;");
    etykietaTooltip->setAttribute(Qt::WA_TransparentForMouseEvents);
    etykietaTooltip->hide();

    connect(ui->comboZakresCzasu, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StatisticsWidget::odswiezWykresAktywnosci);
    connect(ui->comboMetryka, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StatisticsWidget::odswiezWykresAktywnosci);

    connect(ui->comboWidokStatystyk, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StatisticsWidget::onComboWidokChanged);
}

StatisticsWidget::~StatisticsWidget()
{
    delete ui;
}

void StatisticsWidget::odswiezWykresAktywnosci() {
    if (!ui->comboZakresCzasu || !ui->comboMetryka) return;

    int zakres = 30;
    int indexZakresu = ui->comboZakresCzasu->currentIndex();
    if (indexZakresu == 0) zakres = 7;
    else if (indexZakresu == 1) zakres = 30;
    else if (indexZakresu == 2) zakres = 365;

    QString metryka = "czas";
    QString jednostkaWykresu = "[h]";
    int indexMetryki = ui->comboMetryka->currentIndex();
    if (indexMetryki == 1) { metryka = "sesje"; jednostkaWykresu = "sesji"; }
    else if (indexMetryki == 2) { metryka = "jednostki"; jednostkaWykresu = "jednostek"; }

    auto dane = appController.pobierzDaneDlaWykresu(zakres, metryka);

    QStringList unikalneDaty;
    QMap<QString, QMap<QString, double>> mapaSerii;
    QMap<QString, double> sumySerii;
    double sumaGlobalna = 0.0;

    for (const auto& wiersz : dane) {
        QString data = wiersz["data"].toString();
        QString seria = wiersz["seria"].toString();
        double wartosc = wiersz["wartosc"].toDouble();

        if (!unikalneDaty.contains(data)) unikalneDaty.append(data);
        mapaSerii[seria][data] = wartosc;
        sumySerii[seria] += wartosc;
        sumaGlobalna += wartosc;
    }

    double progDzienny = 0.5;

    QSet<QString> samodzielneSerie;
    QMap<QString, double> pozostaleWartosciDzien;
    QMap<QString, QStringList> pozostaleDetaleDzien;

    for (const QString& d : unikalneDaty) {
        for (auto it = mapaSerii.begin(); it != mapaSerii.end(); ++it) {
            QString nazwa = it.key();
            double val = it.value().value(d, 0.0);

            if (val == 0.0) continue;

            if (val >= progDzienny) {
                samodzielneSerie.insert(nazwa);
            } else {
                pozostaleWartosciDzien[d] += val;
                pozostaleDetaleDzien[d].append(QString("- %1: %2 %3").arg(nazwa).arg(QString::number(val, 'f', 1)).arg(jednostkaWykresu));
            }
        }
    }

    QStackedBarSeries *series = new QStackedBarSeries();
    QMap<QString, QStringList> tekstyTooltipow;

    for (const QString& nazwaSerii : samodzielneSerie) {
        QBarSet *set = new QBarSet(nazwaSerii);
        QStringList tooltipyDlaTejGry;

        for (const QString& d : unikalneDaty) {
            double val = mapaSerii[nazwaSerii].value(d, 0.0);

            if (val < progDzienny) {
                val = 0.0;
            }

            *set << val;
            if (val > 0) {
                tooltipyDlaTejGry.append(QString("<b>%1</b><br>Wartość: %2 %3")
                                             .arg(nazwaSerii).arg(QString::number(val, 'f', 1)).arg(jednostkaWykresu));
            } else {
                tooltipyDlaTejGry.append("");
            }
        }
        series->append(set);
        tekstyTooltipow.insert(nazwaSerii, tooltipyDlaTejGry);
    }

    if (!pozostaleWartosciDzien.isEmpty()) {
        QBarSet *setPozostale = new QBarSet("Pozostałe drobnostki");
        setPozostale->setColor(QColor("#95a5a6"));
        QStringList tooltipyDlaPozostalych;

        for (const QString& d : unikalneDaty) {
            double val = pozostaleWartosciDzien.value(d, 0.0);
            *setPozostale << val;

            if (val > 0) {
                QString detale = pozostaleDetaleDzien.value(d).join("<br>");
                tooltipyDlaPozostalych.append(QString("<b>Pozostałe drobnostki</b><br>Łącznie: %1 %2<br>W tym:<br>%3")
                                                  .arg(QString::number(val, 'f', 1)).arg(jednostkaWykresu).arg(detale));
            } else {
                tooltipyDlaPozostalych.append("");
            }
        }
        series->append(setPozostale);
        tekstyTooltipow.insert("Pozostałe drobnostki", tooltipyDlaPozostalych);
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->hide();

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(unikalneDaty);

    axisX->setLabelsAngle(-45);

    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText(metryka == "czas" ? "Godziny [h]" : "Ilość");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    connect(series, &QStackedBarSeries::hovered, this, [this, tekstyTooltipow](bool status, int index, QBarSet *barset) {
        if (status && index >= 0) {
            QString nazwa = barset->label();
            QString tekst = tekstyTooltipow.value(nazwa).value(index);

            if (!tekst.isEmpty()) {
                etykietaTooltip->setText(tekst);
                etykietaTooltip->adjustSize();
                etykietaTooltip->show();
            }
        } else {
            etykietaTooltip->hide();
        }
    });

    QChart *staryWykres = ui->wykresSlupkowyAktywnosci->chart();
    ui->wykresSlupkowyAktywnosci->setChart(chart);
    ui->wykresSlupkowyAktywnosci->setRenderHint(QPainter::Antialiasing);

    if (staryWykres) delete staryWykres;
}

bool StatisticsWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui->wykresSlupkowyAktywnosci->viewport() && event->type() == QEvent::MouseMove) {
        if (!etykietaTooltip->isHidden()) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

            int x = mouseEvent->pos().x() + 15;
            int y = mouseEvent->pos().y() + 15;

            if (x + etykietaTooltip->width() > ui->wykresSlupkowyAktywnosci->viewport()->width()) {
                x = mouseEvent->pos().x() - etykietaTooltip->width() - 15;
            }
            if (y + etykietaTooltip->height() > ui->wykresSlupkowyAktywnosci->viewport()->height()) {
                y = mouseEvent->pos().y() - etykietaTooltip->height() - 15;
            }

            etykietaTooltip->move(x, y);
            etykietaTooltip->raise();
        }
    }
    return QWidget::eventFilter(watched, event);
}

void StatisticsWidget::odswiezDane() {
    onComboWidokChanged(ui->comboWidokStatystyk->currentIndex());
}

void StatisticsWidget::onComboWidokChanged(int index) {
    if (index == 0) {
        odswiezWykresAktywnosci();
    } else {
        //pokazKupkeWstydu();
    }
}
