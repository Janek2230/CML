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
#include <QScrollArea>

StatisticsWidget::StatisticsWidget(AppController& controller, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StatisticsWidget),
    appController(controller)
{
    ui->setupUi(this);

    // zdarzenia myszy trafiają do wewnętrznego pola rysowania wykresu, nie do jego zewnętrznej ramki
    ui->wykresSlupkowyAktywnosci->viewport()->setMouseTracking(true);
    // nasłuchujemy na ruchy myszy żeby przesuwać pływającą etykietę z danymi słupka za kursorem
    ui->wykresSlupkowyAktywnosci->viewport()->installEventFilter(this);

    // etykieta jest umieszczona wewnątrz pola rysowania wykresu, dzięki czemu pojawia się na wierzchu słupków
    etykietaTooltip = new QLabel(ui->wykresSlupkowyAktywnosci->viewport());
    etykietaTooltip->setStyleSheet("background-color: rgba(30, 30, 30, 220); color: white; border: 1px solid #555; border-radius: 4px; padding: 5px;");
    // bez tego flagi etykieta blokowałaby kliknięcia i ruchy myszy, wykres przestałby wykrywać najechanie na słupki
    etykietaTooltip->setAttribute(Qt::WA_TransparentForMouseEvents);
    etykietaTooltip->hide();

    connect(ui->comboZakresCzasu, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StatisticsWidget::odswiezWykresAktywnosci);
    connect(ui->comboMetryka, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StatisticsWidget::odswiezWykresAktywnosci);

    // dane każdej zakładki są odświeżane dopiero przy jej otwarciu, nie wszystkie na raz przy starcie
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

    //Odczytanie wybranego zakresu
    int indexZakresu = ui->comboZakresCzasu->currentIndex();
    int zakres = 30;
    if (indexZakresu == 0) zakres = 7;
    else if (indexZakresu == 1) zakres = 30;
    else if (indexZakresu == 2) zakres = 180;
    else if (indexZakresu == 3) zakres = 365;
    else if (indexZakresu == 4) zakres = 9999; // cała historia

    //Odczytanie wybranej jednostki
    QString metryka = "czas";
    QString jednostkaWykresu = "[h]";
    int indexMetryki = ui->comboMetryka->currentIndex();
    if (indexMetryki == 1) { metryka = "sesje"; jednostkaWykresu = "sesji"; }
    else if (indexMetryki == 2) { metryka = "jednostki"; jednostkaWykresu = "jedn."; }

    const auto dane = appController.pobierzDaneDlaWykresu(zakres, metryka);

    if (dane.isEmpty()) {
        // zamiast pustego miejsca pokazujemy komunikat
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
    QMap<QString, QMap<QString, double>> mapaSerii;

    for (const auto& wiersz : dane) {
        QString data = wiersz.data;
        QString seria = wiersz.nazwaSerii;
        double wartosc = wiersz.wartosc;

        if (!unikalneDaty.contains(data)) unikalneDaty.append(data);

        mapaSerii[seria][data] = wartosc;
    }

    // Stos wg rankingu W OBRĘBIE słupka: dla każdego przedziału czasu osobno sortujemy
    // tytuły malejąco i przypisujemy je do "miejsc" 1..N. Kolor = miejsce w rankingu danego
    // słupka (nie konkretny tytuł), a tytuł i wartość trafiają do tooltipa. Dzięki temu każdy
    // słupek pokazuje SWOICH liderów, a nie globalny top — reszta ląduje w "Pozostałe".
    const int TOP_N = 4;

    QStringList etykietySlotow;
    for (int r = 0; r < TOP_N; ++r) etykietySlotow << QString("%1. miejsce").arg(r + 1);
    etykietySlotow << "Pozostałe";

    const QList<QColor> kolorySlotow = {
        QColor("#f1c40f"),            // 1. miejsce — złoto
        QColor("#bdc3c7"),            // 2. miejsce — srebro
        QColor("#cd7f32"),            // 3. miejsce — brąz
        QColor("#2d89ef"),            // 4. miejsce
        QColor(127, 140, 141, 150)    // Pozostałe
    };

    QStackedBarSeries *series = new QStackedBarSeries();
    QMap<QString, QStringList> tekstyTooltipow;
    QList<QBarSet*> setySlotow;
    for (int s = 0; s < etykietySlotow.size(); ++s) {
        QBarSet* set = new QBarSet(etykietySlotow[s]);
        set->setColor(kolorySlotow[s]);
        setySlotow << set;
        tekstyTooltipow[etykietySlotow[s]] = QStringList();
    }

    for (const QString& d : unikalneDaty) {
        // (tytuł, wartość) dla tego słupka, posortowane malejąco.
        QList<QPair<double, QString>> wDniu;
        for (auto it = mapaSerii.begin(); it != mapaSerii.end(); ++it) {
            const double val = it.value().value(d, 0.0);
            if (val > 0.0) wDniu.append({val, it.key()});
        }
        std::sort(wDniu.begin(), wDniu.end(), [](const QPair<double, QString>& a, const QPair<double, QString>& b) {
            return a.first > b.first;
        });

        // Sloty 1..TOP_N — pojedynczy tytuł na danym miejscu (lub 0 gdy brak).
        for (int r = 0; r < TOP_N; ++r) {
            if (r < wDniu.size()) {
                *setySlotow[r] << wDniu[r].first;
                tekstyTooltipow[etykietySlotow[r]].append(
                    QString("<b>%1. %2</b><br>%3 %4")
                        .arg(r + 1)
                        .arg(wDniu[r].second, QString::number(wDniu[r].first, 'f', 1), jednostkaWykresu));
            } else {
                *setySlotow[r] << 0.0;
                tekstyTooltipow[etykietySlotow[r]].append("");
            }
        }

        // Slot zbiorczy "Pozostałe" — suma i lista tytułów poza TOP_N.
        double sumaReszty = 0.0;
        QString detaleReszty;
        for (int r = TOP_N; r < wDniu.size(); ++r) {
            sumaReszty += wDniu[r].first;
            detaleReszty += QString("- %1: %2 %3<br>").arg(wDniu[r].second, QString::number(wDniu[r].first, 'f', 1), jednostkaWykresu);
        }
        *setySlotow[TOP_N] << sumaReszty;
        if (sumaReszty > 0.0) {
            tekstyTooltipow["Pozostałe"].append(
                QString("<b>Pozostałe (%1 tytułów)</b><br>Łącznie: %2 %3<br>%4")
                    .arg(wDniu.size() - TOP_N)
                    .arg(QString::number(sumaReszty, 'f', 1), jednostkaWykresu, detaleReszty));
        } else {
            tekstyTooltipow["Pozostałe"].append("");
        }
    }

    for (QBarSet* set : setySlotow) series->append(set);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(unikalneDaty);
    axisX->setLabelsAngle(-45); // daty pisane poziomo zachodziłyby na siebie
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
    chart->setBackgroundVisible(false); // przezroczyste tło żeby widoczne było tło rodzica

    // tekstyTooltipow jest przechwycone przez wartość, bo lambda żyje dłużej niż ta funkcja
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
}

bool StatisticsWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui->wykresSlupkowyAktywnosci->viewport() && event->type() == QEvent::MouseMove) {
        if (!etykietaTooltip->isHidden()) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

            // domyślnie etykieta pojawia się 15px w prawo i w dół od kursora, żeby go nie zasłaniać
            int x = mouseEvent->pos().x() + 15;
            int y = mouseEvent->pos().y() + 15;

            // jeśli etykieta wychodziłaby poza prawą lub dolną krawędź, przerzucamy ją na drugą stronę kursora
            if (x + etykietaTooltip->width() > ui->wykresSlupkowyAktywnosci->viewport()->width()) {
                x = mouseEvent->pos().x() - etykietaTooltip->width() - 15;
            }
            if (y + etykietaTooltip->height() > ui->wykresSlupkowyAktywnosci->viewport()->height()) {
                y = mouseEvent->pos().y() - etykietaTooltip->height() - 15;
            }

            etykietaTooltip->move(x, y);
            etykietaTooltip->raise(); // gwarantuje że etykieta nie schowa się pod inne elementy wykresu
        }
    }
    // przekazujemy zdarzenie dalej, żeby Qt mógł je normalnie obsłużyć
    return QWidget::eventFilter(watched, event);
}

void StatisticsWidget::odswiezDane() {
    int index = ui->tabWidget->currentIndex();
    if (index == 0) odswiezPodsumowanieOgolne();
    else if (index == 1) odswiezWykresAktywnosci();
    else if (index == 2) odswiezKupkeWstydu();
    else if (index == 3) odswiezNigdyNieukonczone();
    else if (index == 4) odswiezUlubione();
}

void StatisticsWidget::odswiezNigdyNieukonczone() {
    if (ui->layoutNigdyNieukonczone->layout()) {
        wyczyscLayout(ui->layoutNigdyNieukonczone->layout());
        delete ui->layoutNigdyNieukonczone->layout();
    }

    QGridLayout* siatkaPorzucone = new QGridLayout(ui->layoutNigdyNieukonczone);
    siatkaPorzucone->setAlignment(Qt::AlignTop);

    const auto wszystkie = appController.pobierzWszystkieMultimedia();

    QMap<int, QString> mapaPlatform;
    for (const auto& platforma : appController.pobierzPlatformy()) {
        mapaPlatform.insert(platforma.first, platforma.second);
    }

    bool dodanoCokolwiek = false;
    int row = 0;
    int col = 0;
    const int maxCols = 4; // Liczba kolumn kafelków na szerokość
    // Równe rozciągnięcie kolumn — pełne rzędy wypełniają całą szerokość, bez pustki po prawej.
    for (int c = 0; c < maxCols; ++c) siatkaPorzucone->setColumnStretch(c, 1);

    for (const auto& m : wszystkie) {
        const auto historia = appController.pobierzHistorie(m->getId());
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
    // Pełna przebudowa zakładki: trzy pionowe kolumny, każda z własnym, niezależnym scrollem.
    if (ui->layoutUlubione->layout()) {
        wyczyscLayout(ui->layoutUlubione->layout());
        delete ui->layoutUlubione->layout();
    }

    const auto wszystkie = appController.pobierzWszystkieMultimedia();

    QMap<int, QString> mapaPlatform;
    for (const auto& platforma : appController.pobierzPlatformy()) {
        mapaPlatform.insert(platforma.first, platforma.second);
    }

    // Łączny czas (sekundy) per medium liczymy raz i cache'ujemy.
    QMap<int, long long> czasMap;
    for (const auto& m : wszystkie) {
        long long sumaSekund = 0;
        for (const auto& p : appController.pobierzHistorie(m->getId()))
            for (const auto& s : p.sesje) sumaSekund += s.sekundy;
        czasMap[m->getId()] = sumaSekund;
    }

    // --- Trzy posortowane listy (wszystkie pozycje, bez limitu Top 5) ---
    QList<std::shared_ptr<Multimedia>> ulubione;
    for (const auto& m : wszystkie) if (m->getCzyUlubione()) ulubione.append(m);
    std::sort(ulubione.begin(), ulubione.end(), [&czasMap](const auto& a, const auto& b) {
        return czasMap[a->getId()] > czasMap[b->getId()];
    });

    QList<std::shared_ptr<Multimedia>> rankingCzasu;
    for (const auto& m : wszystkie) if (czasMap[m->getId()] > 0) rankingCzasu.append(m);
    std::sort(rankingCzasu.begin(), rankingCzasu.end(), [&czasMap](const auto& a, const auto& b) {
        return czasMap[a->getId()] > czasMap[b->getId()];
    });

    QList<std::shared_ptr<Multimedia>> rankingPodejsc;
    for (const auto& m : wszystkie) if (m->getPostep().numer_podejscia > 1) rankingPodejsc.append(m);
    std::sort(rankingPodejsc.begin(), rankingPodejsc.end(), [](const auto& a, const auto& b) {
        return a->getPostep().numer_podejscia > b->getPostep().numer_podejscia;
    });

    // --- Trzy kolumny z niezależnym scrollem ---
    QHBoxLayout* glowny = new QHBoxLayout(ui->layoutUlubione);
    glowny->setSpacing(12);

    // Tworzy kolumnę (nagłówek + scroll z pionową listą), podpina ją i zwraca layout listy.
    auto dodajKolumne = [glowny](const QString& tytul) -> QVBoxLayout* {
        QVBoxLayout* kolumna = new QVBoxLayout();
        QLabel* naglowek = new QLabel(tytul);
        naglowek->setStyleSheet("font-size: 15px; font-weight: bold; padding: 4px; color: #ffffff;");
        kolumna->addWidget(naglowek);

        QScrollArea* scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setFrameShape(QFrame::NoFrame);

        QWidget* content = new QWidget();
        QVBoxLayout* lista = new QVBoxLayout(content);
        lista->setAlignment(Qt::AlignTop);
        lista->setSpacing(8);
        scroll->setWidget(content);

        kolumna->addWidget(scroll);
        glowny->addLayout(kolumna);
        return lista;
    };

    // Kolumna 1: oznaczone jako ulubione (bez serduszka w nagłówku).
    QVBoxLayout* listaUlubione = dodajKolumne("Oznaczone jako ulubione");
    for (const auto& m : ulubione) {
        QWidget* k = zbudujKafelek(m, mapaPlatform, false, false);
        QVBoxLayout* l = qobject_cast<QVBoxLayout*>(k->layout());
        if (l && czasMap[m->getId()] > 0)
            l->insertWidget(2, new QLabel(QString("<b>Łącznie:</b> %1").arg(sformatujCzas(czasMap[m->getId()])), k));
        listaUlubione->addWidget(k);
    }
    if (ulubione.isEmpty()) listaUlubione->addWidget(new QLabel("Brak ulubionych tytułów."));
    listaUlubione->addStretch();

    // Kolumna 2: najwięcej spędzonego czasu (wszystkie z czasem > 0).
    QVBoxLayout* listaCzas = dodajKolumne("Najwięcej spędzonego czasu");
    for (const auto& m : rankingCzasu) {
        QWidget* k = zbudujKafelek(m, mapaPlatform, false, false);
        QVBoxLayout* l = qobject_cast<QVBoxLayout*>(k->layout());
        if (l) l->insertWidget(2, new QLabel(QString("<b>Łącznie:</b> %1").arg(sformatujCzas(czasMap[m->getId()])), k));
        listaCzas->addWidget(k);
    }
    if (rankingCzasu.isEmpty()) listaCzas->addWidget(new QLabel("Brak danych o czasie."));
    listaCzas->addStretch();

    // Kolumna 3: najbardziej wytrwałe (wszystkie z więcej niż jednym podejściem).
    QVBoxLayout* listaPodejsc = dodajKolumne("Najbardziej wytrwałe (najwięcej podejść)");
    for (const auto& m : rankingPodejsc) {
        QWidget* k = zbudujKafelek(m, mapaPlatform, false, false);
        QVBoxLayout* l = qobject_cast<QVBoxLayout*>(k->layout());
        if (l) l->insertWidget(2, new QLabel(QString("<b>Podejść:</b> %1").arg(m->getPostep().numer_podejscia), k));
        listaPodejsc->addWidget(k);
    }
    if (rankingPodejsc.isEmpty()) listaPodejsc->addWidget(new QLabel("Brak wielokrotnych podejść."));
    listaPodejsc->addStretch();
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

    ui->tableSummaryTagi->setRowCount(0);
    ui->tableSummaryTagi->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableSummaryTagi->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tableSummaryTagi->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->tableSummaryTagi->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableSummaryTagi->verticalHeader()->setVisible(false);



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

    // Układ dwukolumnowy: zewnętrzny QHBoxLayout trzyma dwie pionowe kolumny kafelków (4+4).
    QHBoxLayout* layoutOsiagniec = new QHBoxLayout(ui->groupBox);
    layoutOsiagniec->setContentsMargins(15, 20, 15, 15);
    layoutOsiagniec->setSpacing(30);

    QVBoxLayout* kolumnaLewa  = new QVBoxLayout();
    QVBoxLayout* kolumnaPrawa = new QVBoxLayout();
    kolumnaLewa->setSpacing(10);
    kolumnaPrawa->setSpacing(10);

    // Ciekawostki oparte o nowe pola: twórca i rok wydania.
    const int dekada = ciekawostki.value("dominujacaDekada", 0).toInt();
    const QString dekadaTekst = dekada > 0 ? QString("Lata %1").arg(dekada) : "Brak danych";
    const QString dekadaPodtytul = dekada > 0
        ? QString("%1 pozycji z tej dekady").arg(ciekawostki.value("dominujacaDekadaIlosc", 0).toInt())
        : "Uzupełnij rok wydania w pozycjach";

    const bool maWiek = ciekawostki.contains("sredniWiekNadrabiania");
    const QString wiekTekst = maWiek
        ? QString("%1 lat").arg(qRound(ciekawostki.value("sredniWiekNadrabiania").toDouble()))
        : "Brak danych";

    // Pora doby → persona. Surowa nazwa pory z bazy ('Noc'/'Rano'/...) mapowana na chwytliwy tytuł.
    const QString pora = ciekawostki.value("poraDoby", "").toString();
    QString poraPersona = "Brak danych";
    QString poraOpis = "Najaktywniejsza pora doby";
    if (pora == "Noc")             { poraPersona = "Nocny Marek";          poraOpis = "Najwięcej czasu w nocy (00–06)"; }
    else if (pora == "Rano")       { poraPersona = "Ranny Ptaszek";        poraOpis = "Najwięcej czasu rano (06–12)"; }
    else if (pora == "Popołudnie") { poraPersona = "Popołudniowy Gracz";   poraOpis = "Najwięcej czasu po południu (12–18)"; }
    else if (pora == "Wieczór")    { poraPersona = "Wieczorny Maratończyk"; poraOpis = "Najwięcej czasu wieczorem (18–24)"; }

    // Kolejność dobrana pod układ dwukolumnowy: lewa = rekordy konsumpcji, prawa = Twój profil/gust.
    struct Osiagniecie { QString tytul; QString wartosc; QString podtytul; };
    QList<Osiagniecie> osiagniecia = {
        // --- Kolumna lewa (rekordy) ---
        {"Maratończyk (Najdłuższa sesja)",
         ciekawostki.value("rekordSesjaTytul", "Brak").toString(),
         sformatujCzas(ciekawostki.value("rekordSesjaCzas", 0).toLongLong())},

        {"Pożeracz Czasu (Najwięcej godzin)",
         ciekawostki.value("pozeraczTytul", "Brak").toString(),
         sformatujCzas(ciekawostki.value("pozeraczCzas", 0).toLongLong())},

        {"Pożeracz Twórców (Najwięcej godzin)",
         ciekawostki.value("ulubionyTworca", "Brak").toString(),
         sformatujCzas(ciekawostki.value("ulubionyTworcaCzas", 0).toLongLong())},

        {"Syndrom 'Jeszcze 1 tury' (Najwięcej podejść)",
         ciekawostki.value("uparyTytul", "Brak").toString(),
         QString("Podejścia: %1").arg(ciekawostki.value("uparyIlosc", 0).toInt())},

        // --- Kolumna prawa (profil / gust) ---
        {"Twój dzień na relaks",
         ciekawostki.value("najlepszyDzien", "Brak").toString(),
         "Najbardziej aktywny dzień tygodnia"},

        {"Twoja pora doby",
         poraPersona,
         poraOpis},

        {"Twoja epoka (Najczęstsza dekada)",
         dekadaTekst,
         dekadaPodtytul},

        {"Wiek nadrabiania",
         wiekTekst,
         "Średnie opóźnienie między premierą a dodaniem"}
    };

    // (size+1)/2 → przy 8 kafelkach daje 4 w lewej, 4 w prawej (i poprawnie dzieli nieparzyste).
    const int progKolumny = (osiagniecia.size() + 1) / 2;
    for (int i = 0; i < osiagniecia.size(); ++i) {
        const auto& os = osiagniecia[i];
        QLabel* lbl = new QLabel(ui->groupBox);
        QString html = QString(
                           "<div style='margin-bottom: 5px;'>"
                           "  <span style='color: #8a8f98; font-size: 12px;'>%1</span><br>"
                           "  <span style='color: #ffffff; font-size: 16px; font-weight: bold;'>%2</span><br>"
                           "  <span style='color: #2d89ef; font-size: 13px; font-weight: bold;'>%3</span>"
                           "</div>"
                           ).arg(os.tytul, os.wartosc, os.podtytul);

        lbl->setText(html);
        lbl->setTextFormat(Qt::RichText);
        (i < progKolumny ? kolumnaLewa : kolumnaPrawa)->addWidget(lbl);
    }

    kolumnaLewa->addStretch();
    kolumnaPrawa->addStretch();
    layoutOsiagniec->addLayout(kolumnaLewa);
    layoutOsiagniec->addLayout(kolumnaPrawa);
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
    kafelek->setMinimumWidth(240);
    // Bez sztywnej maksymalnej szerokości + poziome "Expanding" — kafelek wypełnia
    // szerokość swojej kolumny/komórki zamiast zostawiać pustą przestrzeń obok.
    kafelek->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
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
    const auto wszystkie = appController.pobierzWszystkieMultimedia();
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
