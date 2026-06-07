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

    // zdarzenia myszy trafiają do wewnętrznego pola rysowania wykresu, nie do jego zewnętrznej ramki
    ui->wykresSlupkowyAktywnosci->viewport()->setMouseTracking(true);
    // nasłuchujemy na ruchy myszy żeby przesuwać pływającą etykietę z danymi słupka za kursorem
    ui->wykresSlupkowyAktywnosci->viewport()->installEventFilter(this);

    // etykieta jest umieszczona wewnątrz pola rysowania wykresu, dzięki czemu pojawia się na wierzchu słupków
    etykietaDymka = new QLabel(ui->wykresSlupkowyAktywnosci->viewport());
    etykietaDymka->setStyleSheet("background-color: rgba(30, 30, 30, 220); color: white; border: 1px solid #555; border-radius: 4px; padding: 5px;");
    // bez tego flagi etykieta blokowałaby kliknięcia i ruchy myszy, wykres przestałby wykrywać najechanie na słupki
    etykietaDymka->setAttribute(Qt::WA_TransparentForMouseEvents);
    etykietaDymka->hide();

    connect(ui->comboZakresCzasu, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StatisticsWidget::odswiezWykresAktywnosci);
    connect(ui->comboMetryka, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StatisticsWidget::odswiezWykresAktywnosci);

    // dane każdej zakładki są odświeżane dopiero przy jej otwarciu, nie wszystkie na raz przy starcie
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int) {
        odswiezDane();
    });

    // Tryby rozciągania kolumn/wierszy tabeli tagów są stałe — ustawiamy raz tutaj, a nie przy
    // każdym odświeżeniu zakładki. Tryb per-sekcja nie jest wyrażalny w .ui, więc zostaje w kodzie.
    ui->tableSummaryTagi->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableSummaryTagi->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tableSummaryTagi->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->tableSummaryTagi->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
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
        QChart *pustyWykres = new QChart();
        pustyWykres->setTitle(QString("Brak aktywności w wybranym okresie (%1)")
                                 .arg(ui->comboZakresCzasu->currentText().toLower()));

        QFont font = pustyWykres->titleFont();
        font.setPointSize(14);
        font.setBold(true);
        pustyWykres->setTitleFont(font);
        pustyWykres->setTitleBrush(QBrush(QColor(150, 150, 150)));

        pustyWykres->legend()->hide();
        pustyWykres->setBackgroundVisible(false);

        ui->wykresSlupkowyAktywnosci->setChart(pustyWykres);
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
    // słupek pokazuje SWOICH liderów — reszta ląduje w "Pozostałe".
    const int TOP_N = 4;

    QStringList etykietySlotow;
    for (int r = 0; r < TOP_N; ++r) etykietySlotow << QString("%1. miejsce").arg(r + 1);
    etykietySlotow << "Pozostałe";

    const QList<QColor> kolorySlotow = {
        QColor(0xf1, 0xc4, 0x0f),     // 1. miejsce — złoto
        QColor(0xbd, 0xc3, 0xc7),     // 2. miejsce — srebro
        QColor(0xcd, 0x7f, 0x32),     // 3. miejsce — brąz
        QColor(0x2d, 0x89, 0xef),     // 4. miejsce
        QColor(127, 140, 141, 150)    // Pozostałe
    };

    QStackedBarSeries *series = new QStackedBarSeries();
    QMap<QString, QStringList> tekstyDymkow;
    QList<QBarSet*> setySlotow;
    for (int s = 0; s < etykietySlotow.size(); ++s) {
        QBarSet* set = new QBarSet(etykietySlotow[s]);
        set->setColor(kolorySlotow[s]);
        setySlotow << set;
        tekstyDymkow[etykietySlotow[s]] = QStringList();
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
                tekstyDymkow[etykietySlotow[r]].append(
                    QString("<b>%1. %2</b><br>%3 %4")
                        .arg(r + 1)
                        .arg(wDniu[r].second, QString::number(wDniu[r].first, 'f', 1), jednostkaWykresu));
            } else {
                *setySlotow[r] << 0.0;
                tekstyDymkow[etykietySlotow[r]].append("");
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
            tekstyDymkow["Pozostałe"].append(
                QString("<b>Pozostałe (%1 tytułów)</b><br>Łącznie: %2 %3<br>%4")
                    .arg(wDniu.size() - TOP_N)
                    .arg(QString::number(sumaReszty, 'f', 1), jednostkaWykresu, detaleReszty));
        } else {
            tekstyDymkow["Pozostałe"].append("");
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

    // tekstyDymkow jest przechwycone przez wartość, bo lambda żyje dłużej niż ta funkcja
    connect(series, &QStackedBarSeries::hovered, this, [this, tekstyDymkow](bool status, int index, QBarSet *barset) {
        if (status && index >= 0) {
            QString nazwa = barset->label();
            QString tekst = tekstyDymkow.value(nazwa).value(index);
            if (!tekst.isEmpty()) {
                etykietaDymka->setText(tekst);
                etykietaDymka->adjustSize();
                etykietaDymka->show();
            }
        } else {
            etykietaDymka->hide();
        }
    });

    ui->wykresSlupkowyAktywnosci->setChart(chart);
    ui->wykresSlupkowyAktywnosci->setRenderHint(QPainter::Antialiasing);
    ui->wykresSlupkowyAktywnosci->setBackgroundBrush(Qt::NoBrush);
}

bool StatisticsWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui->wykresSlupkowyAktywnosci->viewport() && event->type() == QEvent::MouseMove) {
        if (!etykietaDymka->isHidden()) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

            // domyślnie etykieta pojawia się 15px w prawo i w dół od kursora, żeby go nie zasłaniać
            int x = mouseEvent->pos().x() + 15;
            int y = mouseEvent->pos().y() + 15;

            // jeśli etykieta wychodziłaby poza prawą lub dolną krawędź, przerzucamy ją na drugą stronę kursora
            if (x + etykietaDymka->width() > ui->wykresSlupkowyAktywnosci->viewport()->width()) {
                x = mouseEvent->pos().x() - etykietaDymka->width() - 15;
            }
            if (y + etykietaDymka->height() > ui->wykresSlupkowyAktywnosci->viewport()->height()) {
                y = mouseEvent->pos().y() - etykietaDymka->height() - 15;
            }

            etykietaDymka->move(x, y);
            etykietaDymka->raise(); // gwarantuje że etykieta nie schowa się pod inne elementy wykresu
        }
    }
    // przekazujemy zdarzenie dalej, żeby Qt mógł je normalnie obsłużyć
    return QWidget::eventFilter(watched, event);
}

void StatisticsWidget::odswiezDane() {
    // Odświeżamy tylko aktywną zakładkę (index = pozycja w tabWidget):
    //   0 = Podsumowanie, 1 = Wykres aktywności, 2 = Kupka Wstydu,
    //   3 = Nigdy nieukończone, 4 = Ulubione.
    int index = ui->tabWidget->currentIndex();
    if (index == 0) odswiezPodsumowanieOgolne();
    else if (index == 1) odswiezWykresAktywnosci();
    else if (index == 2) odswiezKupkeWstydu();
    else if (index == 3) odswiezNigdyNieukonczone();
    else if (index == 4) odswiezUlubione();
}

void StatisticsWidget::odswiezNigdyNieukonczone() {
    // Pusta siatka (siatkaPorzucone, columnstretch=1,1,1,1) jest w .ui — tu tylko czyścimy i wypełniamy.
    wyczyscLayout(ui->siatkaPorzucone);
    ui->siatkaPorzucone->setAlignment(Qt::AlignTop);

    const auto wszystkie = appController.pobierzWszystkieMultimedia();
    const QMap<int, QString> platformy = mapaPlatform();

    bool dodanoCokolwiek = false;
    int row = 0;
    int col = 0;
    const int maxCols = 4; // Liczba kolumn kafelków na szerokość

    for (const auto& m : wszystkie) {
        const auto historia = appController.pobierzHistorie(m->id);
        bool czyKiedykolwiekUkonczone = false;
        bool czyKiedykolwiekPorzucone = false;

        for (const auto& p : historia) {
            if (p.status == Status::Ukonczone) czyKiedykolwiekUkonczone = true;
            if (p.status == Status::Porzucone) czyKiedykolwiekPorzucone = true;
        }

        if (czyKiedykolwiekPorzucone && !czyKiedykolwiekUkonczone) {
            // Dodajemy do siatki: pokazWznow = true, pokazPorzuc = false
            ui->siatkaPorzucone->addWidget(zbudujKafelek(m, platformy, true, false), row, col);
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
        ui->siatkaPorzucone->addWidget(new QLabel("Brak trwale porzuconych tytułów.", this), 0, 0);
    }
}

void StatisticsWidget::odswiezUlubione() {
    wyczyscLayout(ui->listaUlubione);
    wyczyscLayout(ui->listaCzas);
    wyczyscLayout(ui->listaPodejsc);

    const auto wszystkie = appController.pobierzWszystkieMultimedia();
    const QMap<int, QString> platformy = mapaPlatform();
    const QMap<int, long long> czasMap = sumaCzasuNaMedium(wszystkie);

    // Trzy posortowane listy
    QList<std::shared_ptr<Multimedia>> ulubione;
    for (const auto& m : wszystkie) if (m->ulubione) ulubione.append(m);
    std::sort(ulubione.begin(), ulubione.end(), [&czasMap](const auto& a, const auto& b) {
        return czasMap.value(a->id) > czasMap.value(b->id);
    });

    QList<std::shared_ptr<Multimedia>> rankingCzasu;
    for (const auto& m : wszystkie) if (czasMap.value(m->id) > 0) rankingCzasu.append(m);
    std::sort(rankingCzasu.begin(), rankingCzasu.end(), [&czasMap](const auto& a, const auto& b) {
        return czasMap.value(a->id) > czasMap.value(b->id);
    });

    QList<std::shared_ptr<Multimedia>> rankingPodejsc;
    for (const auto& m : wszystkie) if (m->postep.numer_podejscia > 1) rankingPodejsc.append(m);
    std::sort(rankingPodejsc.begin(), rankingPodejsc.end(), [](const auto& a, const auto& b) {
        return a->postep.numer_podejscia > b->postep.numer_podejscia;
    });

    // Kolumna 1: ulubione. Linia "Łącznie" tylko gdy jest zarejestrowany czas.
    for (const auto& m : ulubione) {
        const QString linia = czasMap.value(m->id) > 0
            ? QString("<b>Łącznie:</b> %1").arg(sformatujCzas(czasMap.value(m->id)))
            : QString();
        ui->listaUlubione->addWidget(zbudujKafelek(m, platformy, false, false, linia));
    }
    if (ulubione.isEmpty()) ui->listaUlubione->addWidget(new QLabel("Brak ulubionych tytułów."));
    ui->listaUlubione->addStretch();

    // Kolumna 2: najwięcej spędzonego czasu (wszystkie z czasem > 0).
    for (const auto& m : rankingCzasu) {
        ui->listaCzas->addWidget(zbudujKafelek(m, platformy, false, false,
            QString("<b>Łącznie:</b> %1").arg(sformatujCzas(czasMap.value(m->id)))));
    }
    if (rankingCzasu.isEmpty()) ui->listaCzas->addWidget(new QLabel("Brak danych o czasie."));
    ui->listaCzas->addStretch();

    // Kolumna 3: najbardziej wytrwałe (wszystkie z więcej niż jednym podejściem).
    for (const auto& m : rankingPodejsc) {
        ui->listaPodejsc->addWidget(zbudujKafelek(m, platformy, false, false,
            QString("<b>Podejść:</b> %1").arg(m->postep.numer_podejscia)));
    }
    if (rankingPodejsc.isEmpty()) ui->listaPodejsc->addWidget(new QLabel("Brak wielokrotnych podejść."));
    ui->listaPodejsc->addStretch();
}

QString StatisticsWidget::sformatujCzas(long long sekundy) const {
    const long long h = sekundy / 3600;
    const long long m = (sekundy % 3600) / 60;
    return QString("%1h %2m").arg(h).arg(m);
}

QMap<int, QString> StatisticsWidget::mapaPlatform() const {
    QMap<int, QString> mapa;
    for (const auto& platforma : appController.pobierzPlatformy()) {
        mapa.insert(platforma.first, platforma.second);
    }
    return mapa;
}

QMap<int, long long> StatisticsWidget::sumaCzasuNaMedium(const QList<std::shared_ptr<Multimedia>>& media) const {
    QMap<int, long long> czasMap;
    for (const auto& m : media) {
        long long sumaSekund = 0;
        for (const auto& p : appController.pobierzHistorie(m->id))
            for (const auto& s : p.sesje) sumaSekund += s.sekundy;
        czasMap[m->id] = sumaSekund;
    }
    return czasMap;
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

    // WYKRES KOŁOWY (Kategorie)
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

    // WYKRES RADAROWY (TOP 5 Tagów)
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
    // Wymusza etykiety na wierzchołkach
    axisAngular->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    axisAngular->setStartValue(0);

    // Radar (wielokąt) ma sens dopiero od 3 wierzchołków
    if (nTagow > 2) {
        for (int i = 0; i < nTagow; ++i) {
            const qreal norm = (static_cast<qreal>(topTagi[i].value("czasSekundy").toLongLong()) / static_cast<qreal>(maxCzasTagu)) * 100.0;
            radarSeries->append(i, norm);
            axisAngular->append(topTagi[i].value("tag").toString(), i);
        }

        // Aby zamknąć figurę poprawnie, musimy podać zduplikowany pierwszy punkt
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


    // TABELA TAGÓW — tryby kolumn ustawione raz w konstruktorze, ukrycie nagłówka pionowego w .ui.
    ui->tableSummaryTagi->setRowCount(0);



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


    // CIEKAWOSTKI "Ściana Chwały"
    QVariantMap ciekawostki = appController.pobierzCiekawostkiStatystyczne();

    // Dwukolumnowy szkielet (layoutOsiagniec + kolumnaLewa/kolumnaPrawa) jest w .ui — tu tylko
    // czyścimy obie kolumny i wstawiamy etykiety od nowa. Layoutów NIE kasujemy: należą do Designera.
    wyczyscLayout(ui->kolumnaLewa);
    wyczyscLayout(ui->kolumnaPrawa);

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
        (i < progKolumny ? ui->kolumnaLewa : ui->kolumnaPrawa)->addWidget(lbl);
    }

    ui->kolumnaLewa->addStretch();
    ui->kolumnaPrawa->addStretch();
}


QDateTime StatisticsWidget::wyznaczDateReferencyjna(const std::shared_ptr<Multimedia>& medium) const {
    if (medium->status == Status::Planowane) {
        return medium->dataDodania;
    }

    if (medium->dataOstatniejAktywnosci.isValid()) {
        return medium->dataOstatniejAktywnosci;
    }

    return medium->dataDodania;
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
            // deleteLater() zamiast delete — bezpieczne podczas aktywnego event loop.
            item->widget()->deleteLater();
        } else if (item->layout()) {
            wyczyscLayout(item->layout());
            item->layout()->deleteLater();
        }
        delete item;
    }
}

//Odpowiada za panele medów w zakładkach np. ulubione, kupka wstydu
QWidget* StatisticsWidget::zbudujKafelek(const std::shared_ptr<Multimedia>& medium, const QMap<int, QString>& mapaPlatform, bool pokazWznow, bool pokazPorzuc, const QString& dodatkowaLinia) {
    auto* kafelek = new QFrame(this);
    kafelek->setFrameShape(QFrame::StyledPanel);
    kafelek->setMinimumWidth(240);
    // Bez sztywnej maksymalnej szerokości — kafelek wypełnia szerokość swojej kolumny/komórki
    kafelek->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    kafelek->setStyleSheet("QFrame { border: 1px solid #3a3f44; border-radius: 6px; background-color: rgba(255,255,255,0.03); }");

    auto* layout = new QVBoxLayout(kafelek);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);

    auto* labelTytul = new QLabel(medium->tytul, kafelek);
    labelTytul->setWordWrap(true);
    labelTytul->setStyleSheet("font-weight: 600;");
    layout->addWidget(labelTytul);

    auto* progressBar = new QProgressBar(kafelek);
    const Progress postep = medium->postep;
    int procent = 0;
    if (postep.docelowa > 0) {
        procent = qBound(0, static_cast<int>((static_cast<double>(postep.aktualna) / static_cast<double>(postep.docelowa)) * 100.0), 100);
    }
    progressBar->setRange(0, 100);
    progressBar->setValue(procent);
    progressBar->setTextVisible(false);
    progressBar->setFixedHeight(6);
    layout->addWidget(progressBar);

    // Opcjonalna linia statystyki (np. "Łącznie: 12h 30m") tuż pod paskiem postępu, nad "Platforma".
    if (!dodatkowaLinia.isEmpty()) {
        layout->addWidget(new QLabel(dodatkowaLinia, kafelek));
    }

    auto* labelPlatforma = new QLabel(QString("Platforma: %1").arg(mapaPlatform.value(medium->idPlatformy, "Nieznana")), kafelek);
    layout->addWidget(labelPlatforma);

    auto* labelBezczynnosc = new QLabel(QString("Brak aktywności od: %1 dni").arg(policzDniBezczynnosci(medium)), kafelek);
    layout->addWidget(labelBezczynnosc);

    auto* wierszAkcji = new QHBoxLayout();
    bool dodanoCokolwiek = false;

    if (pokazWznow) {
        auto* btnWznow = new QPushButton("Wznów", kafelek);
        wierszAkcji->addWidget(btnWznow);
        connect(btnWznow, &QPushButton::clicked, this, [this, medium]() {
            const Progress postep = medium->postep;
            if (appController.aktualizujPostep(medium->id, Status::WTrakcie, postep.aktualna, postep.docelowa, medium->ocena)) {
                emit zadaniePokazaniaSzczegolow(medium->id);
                odswiezDane();
            }
        });
        dodanoCokolwiek = true;
    }

    if (pokazPorzuc) {
        auto* btnPorzuc = new QPushButton("Porzuć", kafelek);
        wierszAkcji->addWidget(btnPorzuc);
        connect(btnPorzuc, &QPushButton::clicked, this, [this, medium]() {
            const Progress postep = medium->postep;
            if (appController.aktualizujPostep(medium->id, Status::Porzucone, postep.aktualna, postep.docelowa, medium->ocena)) {
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

    const QMap<int, QString> platformy = mapaPlatform();

    QList<std::shared_ptr<Multimedia>> planowane;
    QList<std::shared_ptr<Multimedia>> przerwane;
    QList<std::shared_ptr<Multimedia>> wstrzymane;
    QList<std::shared_ptr<Multimedia>> wszystkieWKupce;

    for (const auto& medium : wszystkie) {
        if (medium->status == Status::Planowane) {
            planowane.append(medium);
            wszystkieWKupce.append(medium);
        } else if (medium->status == Status::WTrakcie) {
            przerwane.append(medium);
            wszystkieWKupce.append(medium);
        } else if (medium->status == Status::Wstrzymane) {
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
            QString("Najdłuższy przestój: %1 (%2 dni)").arg(najdluzszy->tytul).arg(policzDniBezczynnosci(najdluzszy))
        );
    } else {
        ui->labelNajdluzszyPrzestoj->setText("Najdłuższy przestój: Brak pozycji w kupce");
    }

    wyczyscLayout(ui->sekcjaPlanowaneLayout);
    wyczyscLayout(ui->sekcjaPrzerwaneLayout);
    wyczyscLayout(ui->sekcjaWstrzymaneLayout);

    auto wypelnijSekcje = [this, &platformy](QBoxLayout* layout, const QList<std::shared_ptr<Multimedia>>& sekcja) {
        if (sekcja.isEmpty()) {
            auto* labelPusto = new QLabel("Brak pozycji", this);
            labelPusto->setStyleSheet("color: #9da5ad;");
            layout->addWidget(labelPusto);
            layout->addStretch();
            return;
        }

        for (const auto& medium : sekcja) {
            layout->addWidget(zbudujKafelek(medium, platformy));
        }
        layout->addStretch();
    };

    wypelnijSekcje(ui->sekcjaPlanowaneLayout, planowane);
    wypelnijSekcje(ui->sekcjaPrzerwaneLayout, przerwane);
    wypelnijSekcje(ui->sekcjaWstrzymaneLayout, wstrzymane);

    // Bez disconnect jeden klik wywoływałby lambdę wielokrotnie.
    disconnect(ui->btnProponujAktywnosc, nullptr, this, nullptr);
    connect(ui->btnProponujAktywnosc, &QPushButton::clicked, this, [this, wszystkieWKupce]() {
        if (wszystkieWKupce.isEmpty()) {
            QMessageBox::information(this, "Kupka Wstydu", "Brak pozycji do zaproponowania.");
            return;
        }
        int indeks = QRandomGenerator::global()->bounded(wszystkieWKupce.size());
        emit zadaniePokazaniaSzczegolow(wszystkieWKupce.at(indeks)->id);
    });
}
