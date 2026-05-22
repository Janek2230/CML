#include "statisticswidget.h"
#include "ui_statisticswidget.h"

#include <QChartView>
#include <QStackedBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QPolarChart>
#include <QLineSeries>
#include <QCategoryAxis>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QRandomGenerator>
#include <QMessageBox>
#include <QHeaderView>
#include <algorithm>
#include <QPieSeries>

StatisticsWidget::StatisticsWidget(AppController& controller, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StatisticsWidget),
    appController(controller)
{
    ui->setupUi(this);

    ui->horizontalLayout_4->setStretch(0, 1);
    ui->horizontalLayout_4->setStretch(1, 1);

    ui->verticalLayout_8->setStretch(0, 0);
    ui->verticalLayout_8->setStretch(1, 5);
    ui->verticalLayout_8->setStretch(2, 4);

    ui->wykresSlupkowyAktywnosci->setMouseTracking(true);
    ui->wykresSlupkowyAktywnosci->viewport()->setMouseTracking(true);
    ui->wykresSlupkowyAktywnosci->viewport()->installEventFilter(this);

    etykietaTooltip = new QLabel(ui->wykresSlupkowyAktywnosci->viewport());
    etykietaTooltip->setStyleSheet("background-color: rgba(30, 30, 30, 220); color: white; border: 1px solid #555; border-radius: 4px; padding: 5px;");
    etykietaTooltip->setAttribute(Qt::WA_TransparentForMouseEvents);
    etykietaTooltip->hide();

    connect(ui->comboZakresCzasu, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StatisticsWidget::odswiezWykresAktywnosci);
    connect(ui->comboMetryka, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StatisticsWidget::odswiezWykresAktywnosci);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int) {
        odswiezDane();
    });

}

StatisticsWidget::~StatisticsWidget()
{
    delete ui;
}

void StatisticsWidget::odswiezWykresAktywnosci() {
    if (!ui->comboZakresCzasu || !ui->comboMetryka) return;

    int indexZakresu = ui->comboZakresCzasu->currentIndex();
    int zakres = 30;
    if (indexZakresu == 0) zakres = 7;
    else if (indexZakresu == 1) zakres = 30;
    else if (indexZakresu == 2) zakres = 180;
    else if (indexZakresu == 3) zakres = 365;
    else if (indexZakresu == 4) zakres = 9999; // Cała historia

    QString metryka = "czas";
    QString jednostkaWykresu = "[h]";
    int indexMetryki = ui->comboMetryka->currentIndex();
    if (indexMetryki == 1) { metryka = "sesje"; jednostkaWykresu = "sesji"; }
    else if (indexMetryki == 2) { metryka = "jednostki"; jednostkaWykresu = "jedn."; }

    auto dane = appController.pobierzDaneDlaWykresu(zakres, metryka);

    if (dane.isEmpty()) {
        QChart *pustyChart = new QChart();
        pustyChart->setTitle(QString("Brak aktywności w wybranym okresie (%1)")
                                 .arg(ui->comboZakresCzasu->currentText().toLower()));

        QFont font = pustyChart->titleFont();
        font.setPointSize(14);
        font.setBold(true);
        pustyChart->setTitleFont(font);
        pustyChart->setTitleBrush(QBrush(QColor(150, 150, 150)));

        pustyChart->legend()->hide();
        pustyChart->setBackgroundVisible(false);

        ui->wykresSlupkowyAktywnosci->setChart(pustyChart);
        ui->wykresSlupkowyAktywnosci->setRenderHint(QPainter::Antialiasing);
        return;
    }

    QStringList unikalneDaty;
    QMap<QString, double> wielkoscSlupka; // Suma całkowita dla danego dnia/miesiąca
    QMap<QString, QMap<QString, double>> mapaSerii;

    // Etap A: Zbieranie surowych danych
    for (const auto& wiersz : dane) {
        QString data = wiersz["data"].toString();
        QString seria = wiersz["seria"].toString();
        double wartosc = wiersz["wartosc"].toDouble();

        if (!unikalneDaty.contains(data)) unikalneDaty.append(data);

        mapaSerii[seria][data] = wartosc;
        wielkoscSlupka[data] += wartosc; // Wyliczamy, jak "wysoki" jest cały słupek
    }

    // Etap B: Prosty algorytm Globalnych Liderów
    QMap<QString, double> globalneSumySerii;
    for (auto itSeria = mapaSerii.begin(); itSeria != mapaSerii.end(); ++itSeria) {
        double sumaTotal = 0.0;
        for (double val : itSeria.value()) {
            sumaTotal += val;
        }
        globalneSumySerii[itSeria.key()] = sumaTotal;
    }

    // Sortujemy serie malejąco według całkowitego udziału w zadanym przedziale czasu
    QList<QPair<double, QString>> rankingSerii;
    for (auto it = globalneSumySerii.begin(); it != globalneSumySerii.end(); ++it) {
        if (it.value() > 0) {
            rankingSerii.append({it.value(), it.key()});
        }
    }
    std::sort(rankingSerii.begin(), rankingSerii.end(), [](const QPair<double, QString>& a, const QPair<double, QString>& b) {
        return a.first > b.first;
    });

    // Etap C: Wybór Liderów (max 8 największych aktywności w danym oknie czasowym)
    QStringList nazwyLiderow;
    int maxLiderow = 8; // Przy 8 legenda wygląda estetycznie
    for (int i = 0; i < qMin(maxLiderow, static_cast<int>(rankingSerii.size())); ++i) {
        nazwyLiderow.append(rankingSerii[i].second);
    }

    // Etap D: Budowanie warstw wykresu
    QStackedBarSeries *series = new QStackedBarSeries();
    QMap<QString, QStringList> tekstyTooltipow;
    QMap<QString, QBarSet*> setyLiderow;

    for (const QString& nazwaLidera : nazwyLiderow) {
        setyLiderow[nazwaLidera] = new QBarSet(nazwaLidera);
        tekstyTooltipow[nazwaLidera] = QStringList();
    }

    QBarSet *setInne = new QBarSet("Inne");
    setInne->setColor(QColor(127, 140, 141, 150));
    QStringList tooltipyInne;
    bool czyBylyJakiesInne = false;

    for (const QString& d : unikalneDaty) {
        double sumaInneWDanymDniu = 0.0;
        QString detaleInneDlaDnia = "";

        for (auto it = mapaSerii.begin(); it != mapaSerii.end(); ++it) {
            QString nazwaSerii = it.key();
            double val = it.value().value(d, 0.0);

            if (val == 0.0) {
                if (nazwyLiderow.contains(nazwaSerii)) {
                    *setyLiderow[nazwaSerii] << 0.0;
                    tekstyTooltipow[nazwaSerii].append("");
                }
                continue;
            }

            if (nazwyLiderow.contains(nazwaSerii)) {
                *setyLiderow[nazwaSerii] << val;
                tekstyTooltipow[nazwaSerii].append(QString("<b>%1</b><br>Wartość: %2 %3")
                                                       .arg(nazwaSerii).arg(QString::number(val, 'f', 1)).arg(jednostkaWykresu));
            } else {
                sumaInneWDanymDniu += val;
                detaleInneDlaDnia += QString("- %1: %2 %3<br>").arg(nazwaSerii).arg(QString::number(val, 'f', 1)).arg(jednostkaWykresu);
                czyBylyJakiesInne = true;
            }
        }

        *setInne << sumaInneWDanymDniu;
        if (sumaInneWDanymDniu > 0) {
            tooltipyInne.append(QString("<b>Inne / Pozostałe</b><br>Łącznie: %1 %2<br>W tym:<br>%3")
                                    .arg(QString::number(sumaInneWDanymDniu, 'f', 1)).arg(jednostkaWykresu).arg(detaleInneDlaDnia));
        } else {
            tooltipyInne.append("");
        }
    }

    for (const QString& nazwaLidera : nazwyLiderow) {
        series->append(setyLiderow[nazwaLidera]);
    }
    if (czyBylyJakiesInne) {
        series->append(setInne);
        tekstyTooltipow.insert("Inne", tooltipyInne);
    } else {
        delete setInne;
    }

    // Etap E: Rysowanie na ekranie
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(unikalneDaty);
    axisX->setLabelsAngle(-45);
    axisX->setLabelsColor(Qt::white);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText(metryka == "czas" ? "Godziny [h]" : "Ilość");
    axisY->setLabelsColor(Qt::white);
    axisY->setTitleBrush(QBrush(Qt::white));

    QPen gridPen(QColor(255, 255, 255, 40));
    axisY->setGridLinePen(gridPen);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setLabelColor(Qt::white);
    chart->setBackgroundVisible(false);

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

    ui->wykresSlupkowyAktywnosci->setChart(chart);
    ui->wykresSlupkowyAktywnosci->setRenderHint(QPainter::Antialiasing);
    ui->wykresSlupkowyAktywnosci->setBackgroundBrush(Qt::NoBrush);
    ui->wykresSlupkowyAktywnosci->setStyleSheet("background: transparent; border: none;");
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
    int index = ui->tabWidget->currentIndex();
    if (index == 0) odswiezPodsumowanieOgolne();
    else if (index == 1) odswiezWykresAktywnosci();
    else if (index == 2) odswiezKupkeWstydu();
    else if (index == 3) odswiezPorzucone();
    else if (index == 4) odswiezUlubione();
}

void StatisticsWidget::odswiezPorzucone() {
    // Wyrzucamy stary układ do kosza
    if (ui->layoutUlubione->layout()) {
        wyczyscLayout(ui->layoutUlubione->layout());
        delete ui->layoutUlubione->layout();
    }

    // Inicjalizujemy siatkę
    QGridLayout* siatkaPorzucone = new QGridLayout(ui->layoutUlubione);
    siatkaPorzucone->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    auto wszystkie = appController.pobierzWszystkieMultimedia();

    QMap<int, QString> mapaPlatform;
    for (const auto& platforma : appController.pobierzPlatformy()) {
        mapaPlatform.insert(platforma.first, platforma.second);
    }

    bool dodanoCokolwiek = false;
    int row = 0;
    int col = 0;
    const int maxCols = 3; // Liczba kolumn kafelków na szerokość

    for (const auto& m : wszystkie) {
        auto historia = appController.pobierzHistorie(m->getId());
        bool czyKiedykolwiekUkonczone = false;
        bool czyKiedykolwiekPorzucone = false;

        for (const auto& p : historia) {
            if (p.status == "Ukończone") czyKiedykolwiekUkonczone = true;
            if (p.status == "Porzucone") czyKiedykolwiekPorzucone = true;
        }

        if (czyKiedykolwiekPorzucone && !czyKiedykolwiekUkonczone) {
            // Dodajemy do siatki: pokazWznow = true, pokazPorzuc = false
            siatkaPorzucone->addWidget(zbudujKafelek(m, mapaPlatform, true, false), row, col);
            dodanoCokolwiek = true;

            // Logika zawijania wierszy
            col++;
            if (col >= maxCols) {
                col = 0;
                row++;
            }
        }
    }

    if (!dodanoCokolwiek) {
        siatkaPorzucone->addWidget(new QLabel("Brak trwale porzuconych tytułów.", this), 0, 0);
    }
}

void StatisticsWidget::odswiezUlubione() {
    // Uwaga: Używamy 'layoutPorzucone', ponieważ w pliku .ui to ten layout znajduje się w zakładce "Ulubione"
    QVBoxLayout* layoutUlubione = qobject_cast<QVBoxLayout*>(ui->layoutPorzucone->layout());
    if (!layoutUlubione) {
        layoutUlubione = new QVBoxLayout(ui->layoutPorzucone);
    }
    wyczyscLayout(layoutUlubione);

    auto wszystkie = appController.pobierzWszystkieMultimedia();

    QMap<int, QString> mapaPlatform;
    for (const auto& platforma : appController.pobierzPlatformy()) {
        mapaPlatform.insert(platforma.first, platforma.second);
    }

    // --- SEKCJA 1: Oznaczone serduszkiem ---
    layoutUlubione->addWidget(new QLabel("<h2>Tytuły oznaczone jako Ulubione (❤)</h2>"));
    QHBoxLayout* wierszUlubionych = new QHBoxLayout();
    bool czySaUlubione = false;
    for (const auto& m : wszystkie) {
        if (m->getCzyUlubione()) {
            wierszUlubionych->addWidget(zbudujKafelek(m, mapaPlatform, false, false));
            czySaUlubione = true;
        }
    }
    if (!czySaUlubione) wierszUlubionych->addWidget(new QLabel("Brak ulubionych tytułów.", this));
    wierszUlubionych->addStretch();
    layoutUlubione->addLayout(wierszUlubionych);

    // --- SEKCJA 2: Najwięcej czasu ---
    layoutUlubione->addWidget(new QLabel("<br><h2>Najwięcej spędzonego czasu (Top 5)</h2>"));
    QList<QPair<long long, std::shared_ptr<Multimedia>>> rankingCzasu;
    for (const auto& m : wszystkie) {
        long long sumaSekund = 0;
        auto historia = appController.pobierzHistorie(m->getId());
        for (const auto& p : historia) {
            for (const auto& s : p.sesje) sumaSekund += s.sekundy;
        }
        if (sumaSekund > 0) rankingCzasu.append({sumaSekund, m});
    }

    std::sort(rankingCzasu.begin(), rankingCzasu.end(), [](const auto& a, const auto& b) { return a.first > b.first; });

    QHBoxLayout* wierszCzasu = new QHBoxLayout();
    for (int i = 0; i < qMin(5, static_cast<int>(rankingCzasu.size())); ++i) {
        auto m = rankingCzasu[i].second;
        QWidget* kafelek = zbudujKafelek(m, mapaPlatform, false, false);
        QVBoxLayout* l = qobject_cast<QVBoxLayout*>(kafelek->layout());
        if (l) l->insertWidget(2, new QLabel(QString("<b>Łącznie:</b> %1").arg(sformatujCzas(rankingCzasu[i].first)), kafelek));
        wierszCzasu->addWidget(kafelek);
    }
    if (rankingCzasu.isEmpty()) wierszCzasu->addWidget(new QLabel("Brak danych o czasie.", this));
    wierszCzasu->addStretch();
    layoutUlubione->addLayout(wierszCzasu);

    // --- SEKCJA 3: Najwięcej podejść ---
    layoutUlubione->addWidget(new QLabel("<br><h2>Najbardziej wytrwałe (Najwięcej podejść - Top 5)</h2>"));
    QList<std::shared_ptr<Multimedia>> rankingPodejsc = wszystkie;
    std::sort(rankingPodejsc.begin(), rankingPodejsc.end(), [](const std::shared_ptr<Multimedia>& a, const std::shared_ptr<Multimedia>& b) {
        return a->getPostep().numer_podejscia > b->getPostep().numer_podejscia;
    });

    QHBoxLayout* wierszPodejsc = new QHBoxLayout();
    for (int i = 0; i < qMin(5, static_cast<int>(rankingPodejsc.size())); ++i) {
        if (rankingPodejsc[i]->getPostep().numer_podejscia > 1) {
            QWidget* kafelek = zbudujKafelek(rankingPodejsc[i], mapaPlatform, false, false);
            QVBoxLayout* l = qobject_cast<QVBoxLayout*>(kafelek->layout());
            if (l) l->insertWidget(2, new QLabel(QString("<b>Podejść:</b> %1").arg(rankingPodejsc[i]->getPostep().numer_podejscia), kafelek));
            wierszPodejsc->addWidget(kafelek);
        }
    }
    if (wierszPodejsc->count() == 0) wierszPodejsc->addWidget(new QLabel("Nigdy nie dawałeś niczemu drugiej szansy.", this));
    wierszPodejsc->addStretch();
    layoutUlubione->addLayout(wierszPodejsc);
    layoutUlubione->addStretch();
}

QString StatisticsWidget::sformatujCzas(long long sekundy) const {
    const long long h = sekundy / 3600;
    const long long m = (sekundy % 3600) / 60;
    return QString("%1h %2m").arg(h).arg(m);
}

void StatisticsWidget::odswiezPodsumowanieOgolne() {
    const QVariantMap stats = appController.pobierzStatystykiPodsumowania();

    // Lambda formatuje pojedynczy kafel KPI jako wycentrowany HTML.
    auto formatujKPI = [](const QString& tytul, const QString& wartosc) {
        return QString(
                   "<div style='text-align: center; margin-top: 5px;'>"
                   "  <span style='color: #8a8f98; font-size: 10px; text-transform: uppercase;'>%1: </span>"
                   "  <span style='color: #2d89ef; font-size: 12px; font-weight: bold;'>%2</span>"
                   "</div>"
                   ).arg(tytul, wartosc);
    };

    ui->lblSummaryMediaTotal->setText(formatujKPI("W bibliotece", QString::number(stats.value("mediaTotal", 0).toInt())));
    ui->lblSummaryPodejsciaTotal->setText(formatujKPI("Podejścia", QString::number(stats.value("podejsciaTotal", 0).toInt())));
    ui->lblSummarySesjeTotal->setText(formatujKPI("Rozegrane sesje", QString::number(stats.value("sesjeTotal", 0).toInt())));
    ui->lblSummaryCzasTotal->setText(formatujKPI("Spędzony czas", sformatujCzas(stats.value("czasTotal", 0).toLongLong())));
    ui->lblSummaryJednostkiTotal->setText(formatujKPI("Zdobyte jednostki", QString::number(stats.value("jednostkiTotal", 0).toLongLong())));
    ui->lblSummaryUlubionyTag->setText(formatujKPI("Ulubiony tag", stats.value("ulubionyTag", "Brak danych").toString()));

    ui->lblSummaryMediaTotal->setAlignment(Qt::AlignCenter);
    ui->lblSummaryPodejsciaTotal->setAlignment(Qt::AlignCenter);
    ui->lblSummarySesjeTotal->setAlignment(Qt::AlignCenter);
    ui->lblSummaryCzasTotal->setAlignment(Qt::AlignCenter);
    ui->lblSummaryJednostkiTotal->setAlignment(Qt::AlignCenter);
    ui->lblSummaryUlubionyTag->setAlignment(Qt::AlignCenter);

    // Ograniczenie wysokości panelu KPI zapobiega rozciąganiu się go kosztem wykresów poniżej.
    ui->groupBoxKPI->setMaximumHeight(75);

    // ==========================================
    // WYKRES KOŁOWY (Kategorie)
    // ==========================================
    const QList<QVariantMap> kategorie = appController.pobierzPodsumowanieKategorii();
    QPieSeries *pie = new QPieSeries();

    for (const auto& kat : kategorie) {
        const long long sekundy = kat.value("czasSekundy").toLongLong();
        if (sekundy > 0) {
            pie->append(QString("%1 (%2)").arg(kat.value("kategoria").toString(), sformatujCzas(sekundy)), static_cast<qreal>(sekundy));
        }
    }
    if (pie->slices().isEmpty()) pie->append("Brak danych", 1);

    QChart *chartPie = new QChart();
    chartPie->addSeries(pie);
    chartPie->legend()->setVisible(true);
    chartPie->legend()->setAlignment(Qt::AlignRight);
    chartPie->legend()->setLabelColor(Qt::white);

    chartPie->setTitle("");
    chartPie->setMargins(QMargins(0, 0, 0, 0));
    chartPie->setBackgroundRoundness(0);
    chartPie->setBackgroundVisible(false);

    ui->chartSummaryKategorie->setChart(chartPie);
    ui->chartSummaryKategorie->setRenderHint(QPainter::Antialiasing);

    // ==========================================
    // WYKRES RADAROWY (TOP 5 Tagów)
    // ==========================================
    const QList<QVariantMap> tagi = appController.pobierzPodsumowanieTagow();
    auto* radarSeries = new QLineSeries();
    radarSeries->setName("Profil");

    QList<QVariantMap> topTagi;
    long long maxCzasTagu = 1;
    for (int i = 0; i < qMin(5, tagi.size()); ++i) {
        topTagi.append(tagi[i]);
        maxCzasTagu = qMax(maxCzasTagu, tagi[i].value("czasSekundy").toLongLong());
    }

    int nTagow = topTagi.size();
    auto* axisAngular = new QCategoryAxis();
    axisAngular->setLabelsColor(Qt::white);
    // Wymusza etykiety na wierzchołkach, a nie pomiędzy nimi!
    axisAngular->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    axisAngular->setStartValue(0);

    if (nTagow > 2) {
        for (int i = 0; i < nTagow; ++i) {
            const qreal norm = (static_cast<qreal>(topTagi[i].value("czasSekundy").toLongLong()) / static_cast<qreal>(maxCzasTagu)) * 100.0;
            // Dodajemy punkty na współrzędnych osi 0, 1, 2...
            radarSeries->append(i, norm);
            axisAngular->append(topTagi[i].value("tag").toString(), i);
        }

        // Aby zamknąć figurę poprawnie, musimy podać zduplikowany pierwszy punkt
        // na kącie odpowiadającym 360 stopni (w naszej skali to 'nTagow', a nie 0).
        const qreal normZamkniecie = (static_cast<qreal>(topTagi[0].value("czasSekundy").toLongLong()) / static_cast<qreal>(maxCzasTagu)) * 100.0;
        radarSeries->append(nTagow, normZamkniecie);

        axisAngular->setRange(0, nTagow);
    } else {
        radarSeries->append(0,0);
        axisAngular->append("Mało danych", 1);
        axisAngular->setRange(0, 1);
    }

    auto* polar = new QPolarChart();
    polar->addSeries(radarSeries);
    polar->legend()->setVisible(false);
    polar->setTitle("");
    polar->setMargins(QMargins(0, 0, 0, 0));
    polar->setBackgroundVisible(false);

    polar->addAxis(axisAngular, QPolarChart::PolarOrientationAngular);
    radarSeries->attachAxis(axisAngular);

    auto* axisRadial = new QValueAxis();
    axisRadial->setRange(0, 100);
    axisRadial->setVisible(false); // Ukrywamy siatkę cyferek, żeby nie śmieciła
    polar->addAxis(axisRadial, QPolarChart::PolarOrientationRadial);
    radarSeries->attachAxis(axisRadial);

    ui->chartSummaryRadar->setChart(polar);
    ui->chartSummaryRadar->setRenderHint(QPainter::Antialiasing);

    // ==========================================
    // TABELA TAGÓW (Ucywilizowana)
    // ==========================================

    if (!ui->groupBoxTagi->layout()) {
        QVBoxLayout *layoutTagi = new QVBoxLayout(ui->groupBoxTagi);
        layoutTagi->setContentsMargins(10, 20, 10, 10);
        layoutTagi->addWidget(ui->tableSummaryTagi);
    }

    ui->tableSummaryTagi->setRowCount(0);
    ui->tableSummaryTagi->setColumnCount(3);
    ui->tableSummaryTagi->setHorizontalHeaderLabels({"Tag", "Wpisy", "Czas"});
    ui->tableSummaryTagi->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableSummaryTagi->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tableSummaryTagi->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->tableSummaryTagi->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableSummaryTagi->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableSummaryTagi->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableSummaryTagi->verticalHeader()->setVisible(false);
    ui->tableSummaryTagi->setShowGrid(false);
    ui->tableSummaryTagi->setStyleSheet("QTableWidget { border: none; background-color: transparent; } QHeaderView::section { background-color: transparent; border: none; font-weight: bold; color: #a0a0a0; border-bottom: 1px solid #3a3f44; padding: 4px; } QTableWidget::item { border-bottom: 1px solid rgba(255,255,255,0.05); padding: 4px; }");



    const int limitTagow = qMin(10, tagi.size());
    for (int i = 0; i < limitTagow; ++i) {
        ui->tableSummaryTagi->insertRow(i);
        const auto& t = tagi.at(i);
        ui->tableSummaryTagi->setItem(i, 0, new QTableWidgetItem(t.value("tag").toString()));

        auto* itemMedia = new QTableWidgetItem(QString::number(t.value("liczbaMediow").toInt()));
        itemMedia->setTextAlignment(Qt::AlignCenter);
        ui->tableSummaryTagi->setItem(i, 1, itemMedia);

        auto* itemCzas = new QTableWidgetItem(sformatujCzas(t.value("czasSekundy").toLongLong()));
        itemCzas->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ui->tableSummaryTagi->setItem(i, 2, itemCzas);
    }

    // ==========================================
    // CIEKAWOSTKI ("Ściana Chwały" 2.0)
    // ==========================================
    QVariantMap ciekawostki = appController.pobierzCiekawostkiStatystyczne();

    // groupBox ma domyślny layout z Qt Designera — czyścimy go przed ponownym wypełnieniem.
    QLayout* staryLayout = ui->groupBox->layout();
    if (staryLayout) {
        QLayoutItem* item;
        while ((item = staryLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                item->widget()->hide();
                item->widget()->deleteLater();
            }
            delete item;
        }
        delete staryLayout;
    }

    QVBoxLayout* layoutOsiagniec = new QVBoxLayout(ui->groupBox);
    layoutOsiagniec->setContentsMargins(15, 20, 15, 15);
    layoutOsiagniec->setSpacing(10);

    struct Osiagniecie { QString ikona; QString tytul; QString wartosc; QString podtytul; };
    QList<Osiagniecie> osiagniecia = {
        {"", "Maratończyk (Najdłuższa sesja)",
         ciekawostki.value("rekordSesjaTytul", "Brak").toString(),
         sformatujCzas(ciekawostki.value("rekordSesjaCzas", 0).toLongLong())},

        {"", "Pożeracz Czasu (Najwięcej godzin)",
         ciekawostki.value("pozeraczTytul", "Brak").toString(),
         sformatujCzas(ciekawostki.value("pozeraczCzas", 0).toLongLong())},

        {"", "Syndrom 'Jeszcze 1 tury' (Najwięcej podejść)",
         ciekawostki.value("uparyTytul", "Brak").toString(),
         QString("%1 podejść").arg(ciekawostki.value("uparyIlosc", 0).toInt())},

        {"","Twój dzień na relaks",
         ciekawostki.value("najlepszyDzien", "Brak").toString(),
         "Najbardziej aktywny dzień tygodnia"}
    };

    for (const auto& os : osiagniecia) {
        QLabel* lbl = new QLabel(ui->groupBox);
        QString html = QString(
                           "<div style='margin-bottom: 5px;'>"
                           "  <span style='color: #8a8f98; font-size: 12px;'>%1 %2</span><br>"
                           "  <span style='color: #ffffff; font-size: 16px; font-weight: bold;'>%3</span><br>"
                           "  <span style='color: #2d89ef; font-size: 13px; font-weight: bold;'>%4</span>"
                           "</div>"
                           ).arg(os.ikona, os.tytul, os.wartosc, os.podtytul);

        lbl->setText(html);
        lbl->setTextFormat(Qt::RichText);
        layoutOsiagniec->addWidget(lbl);
    }

    layoutOsiagniec->addStretch();
}


QDateTime StatisticsWidget::wyznaczDateReferencyjna(const std::shared_ptr<Multimedia>& medium) const {
    if (medium->getStatus() == "Planowane") {
        return medium->getDataDodania();
    }

    if (medium->getDataOstatniejAktywnosci().isValid()) {
        return medium->getDataOstatniejAktywnosci();
    }

    return medium->getDataDodania();
}

int StatisticsWidget::policzDniBezczynnosci(const std::shared_ptr<Multimedia>& medium) const {
    QDateTime dataReferencyjna = wyznaczDateReferencyjna(medium);
    if (!dataReferencyjna.isValid()) {
        return 0;
    }
    return dataReferencyjna.daysTo(QDateTime::currentDateTime());
}

void StatisticsWidget::wyczyscLayout(QLayout* layout) {
    if (!layout) {
        return;
    }

    QLayoutItem* item = nullptr;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            // deleteLater() zamiast delete — bezpieczne podczas aktywnego event loop Qt.
            item->widget()->deleteLater();
        } else if (item->layout()) {
            wyczyscLayout(item->layout());
            item->layout()->deleteLater();
        }
        delete item;
    }
}

QWidget* StatisticsWidget::zbudujKafelek(const std::shared_ptr<Multimedia>& medium, const QMap<int, QString>& mapaPlatform, bool pokazWznow, bool pokazPorzuc) {
    auto* kafelek = new QFrame(this);
    kafelek->setFrameShape(QFrame::StyledPanel);
    kafelek->setMinimumWidth(260);
    kafelek->setMaximumWidth(300);
    kafelek->setStyleSheet("QFrame { border: 1px solid #3a3f44; border-radius: 6px; background-color: rgba(255,255,255,0.03); }");

    auto* layout = new QVBoxLayout(kafelek);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);

    auto* labelTytul = new QLabel(medium->getTytul(), kafelek);
    labelTytul->setWordWrap(true);
    labelTytul->setStyleSheet("font-weight: 600;");
    layout->addWidget(labelTytul);

    auto* progressBar = new QProgressBar(kafelek);
    const Postep postep = medium->getPostep();
    int procent = 0;
    if (postep.docelowa > 0) {
        procent = qBound(0, static_cast<int>((static_cast<double>(postep.aktualna) / static_cast<double>(postep.docelowa)) * 100.0), 100);
    }
    progressBar->setRange(0, 100);
    progressBar->setValue(procent);
    progressBar->setTextVisible(false);
    progressBar->setFixedHeight(6);
    layout->addWidget(progressBar);

    auto* labelPlatforma = new QLabel(QString("Platforma: %1").arg(mapaPlatform.value(medium->getIdPlatformy(), "Nieznana")), kafelek);
    layout->addWidget(labelPlatforma);

    auto* labelBezczynnosc = new QLabel(QString("Brak aktywności od: %1 dni").arg(policzDniBezczynnosci(medium)), kafelek);
    layout->addWidget(labelBezczynnosc);

    auto* wierszAkcji = new QHBoxLayout();
    bool dodanoCokolwiek = false;

    if (pokazWznow) {
        auto* btnWznow = new QPushButton("Wznów", kafelek);
        wierszAkcji->addWidget(btnWznow);
        connect(btnWznow, &QPushButton::clicked, this, [this, medium]() {
            const Postep postep = medium->getPostep();
            if (appController.aktualizujPostep(medium->getId(), "W trakcie", postep.aktualna, postep.docelowa, medium->getOcena())) {
                emit zadaniePokazaniaSzczegolow(medium->getId());
                odswiezDane();
            }
        });
        dodanoCokolwiek = true;
    }

    if (pokazPorzuc) {
        auto* btnPorzuc = new QPushButton("Porzuć", kafelek);
        wierszAkcji->addWidget(btnPorzuc);
        connect(btnPorzuc, &QPushButton::clicked, this, [this, medium]() {
            const Postep postep = medium->getPostep();
            if (appController.aktualizujPostep(medium->getId(), "Porzucone", postep.aktualna, postep.docelowa, medium->getOcena())) {
                odswiezDane();
            }
        });
        dodanoCokolwiek = true;
    }

    if (dodanoCokolwiek) {
        layout->addLayout(wierszAkcji);
    } else {
        delete wierszAkcji;
    }

    return kafelek;
}

void StatisticsWidget::odswiezKupkeWstydu() {
    auto wszystkie = appController.pobierzWszystkieMultimedia();
    const int suma = wszystkie.size();
    if (suma == 0) {
        ui->labelWskaznikZaleglosci->setText("Wskaźnik zaległości: 0 / 0 (0%)");
        ui->labelNajdluzszyPrzestoj->setText("Najdłuższy przestój: Brak danych");
        wyczyscLayout(ui->sekcjaPlanowaneLayout);
        wyczyscLayout(ui->sekcjaPrzerwaneLayout);
        wyczyscLayout(ui->sekcjaWstrzymaneLayout);
        return;
    }

    QMap<int, QString> mapaPlatform;
    for (const auto& platforma : appController.pobierzPlatformy()) {
        mapaPlatform.insert(platforma.first, platforma.second);
    }

    QList<std::shared_ptr<Multimedia>> planowane;
    QList<std::shared_ptr<Multimedia>> przerwane;
    QList<std::shared_ptr<Multimedia>> wstrzymane;
    QList<std::shared_ptr<Multimedia>> wszystkieWKupce;

    for (const auto& medium : wszystkie) {
        if (medium->getStatus() == "Planowane") {
            planowane.append(medium);
            wszystkieWKupce.append(medium);
        } else if (medium->getStatus() == "W trakcie") {
            przerwane.append(medium);
            wszystkieWKupce.append(medium);
        } else if (medium->getStatus() == "Wstrzymane") {
            wstrzymane.append(medium);
            wszystkieWKupce.append(medium);
        }
    }

    auto porownajPoStarosci = [this](const std::shared_ptr<Multimedia>& a, const std::shared_ptr<Multimedia>& b) {
        return wyznaczDateReferencyjna(a) < wyznaczDateReferencyjna(b);
    };
    std::sort(planowane.begin(), planowane.end(), porownajPoStarosci);
    std::sort(przerwane.begin(), przerwane.end(), porownajPoStarosci);
    std::sort(wstrzymane.begin(), wstrzymane.end(), porownajPoStarosci);

    const int iloscWkupce = wszystkieWKupce.size();
    const int procent = static_cast<int>((static_cast<double>(iloscWkupce) / static_cast<double>(suma)) * 100.0);
    ui->labelWskaznikZaleglosci->setText(
        QString("Wskaźnik zaległości: %1 / %2 (%3%)").arg(iloscWkupce).arg(suma).arg(procent)
    );

    if (!wszystkieWKupce.isEmpty()) {
        auto najdluzszy = *std::max_element(wszystkieWKupce.begin(), wszystkieWKupce.end(),
            [this](const std::shared_ptr<Multimedia>& a, const std::shared_ptr<Multimedia>& b) {
                return policzDniBezczynnosci(a) < policzDniBezczynnosci(b);
            });
        ui->labelNajdluzszyPrzestoj->setText(
            QString("Najdłuższy przestój: %1 (%2 dni)").arg(najdluzszy->getTytul()).arg(policzDniBezczynnosci(najdluzszy))
        );
    } else {
        ui->labelNajdluzszyPrzestoj->setText("Najdłuższy przestój: Brak pozycji w kupce");
    }

    wyczyscLayout(ui->sekcjaPlanowaneLayout);
    wyczyscLayout(ui->sekcjaPrzerwaneLayout);
    wyczyscLayout(ui->sekcjaWstrzymaneLayout);

    auto wypelnijSekcje = [this, &mapaPlatform](QBoxLayout* layout, const QList<std::shared_ptr<Multimedia>>& sekcja) {
        if (sekcja.isEmpty()) {
            auto* labelPusto = new QLabel("Brak pozycji", this);
            labelPusto->setStyleSheet("color: #9da5ad;");
            layout->addWidget(labelPusto);
            layout->addStretch();
            return;
        }

        for (const auto& medium : sekcja) {
            layout->addWidget(zbudujKafelek(medium, mapaPlatform));
        }
        layout->addStretch();
    };

    wypelnijSekcje(ui->sekcjaPlanowaneLayout, planowane);
    wypelnijSekcje(ui->sekcjaPrzerwaneLayout, przerwane);
    wypelnijSekcje(ui->sekcjaWstrzymaneLayout, wstrzymane);

    disconnect(ui->btnProponujAktywnosc, nullptr, this, nullptr);
    connect(ui->btnProponujAktywnosc, &QPushButton::clicked, this, [this, wszystkieWKupce]() {
        if (wszystkieWKupce.isEmpty()) {
            QMessageBox::information(this, "Kupka Wstydu", "Brak pozycji do zaproponowania.");
            return;
        }
        int indeks = QRandomGenerator::global()->bounded(wszystkieWKupce.size());
        emit zadaniePokazaniaSzczegolow(wszystkieWKupce.at(indeks)->getId());
    });
}
