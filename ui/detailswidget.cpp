#include "detailswidget.h"
#include "ui_detailswidget.h"
#include <QTextBrowser>
#include <QMessageBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QTextEdit>
#include <QLabel>
#include <QFont>
#include <QTimer>
#include <QGroupBox>
#include <QIcon>
#include <QSize>

namespace {
// Klucze do ukrytych danych węzłów drzewa historii (niewidocznych dla użytkownika).
// Dzięki nim po kliknięciu węzła wiemy: czym jest, jakie ma ID i do którego podejścia należy.
constexpr int ROLA_TYP = Qt::UserRole + 1; // czy węzeł to Podejście czy Sesja
constexpr int ROLA_ID = Qt::UserRole + 2;  // ID rekordu w bazie
}

DetailsWidget::DetailsWidget(AppController& controller, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DetailsWidget),
    appController(controller)
{
    ui->setupUi(this);

    // Przyciski widoczne tylko w konkretnych kontekstach — domyślnie ukryte.
    ui->btnCofnijZakonczenie->hide();
    ui->treeHistoria->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);


    // Zmiana aktualnej wartości - przelicz pasek postępu + auto-zmień status z "Planowane" na "W trakcie".
    connect(ui->spinDetaleAktualny, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        int maxVal = ui->spinDetaleDocelowy->value();
        if (maxVal > 0) {
            int procent = (static_cast<double>(val) / maxVal) * 100;
            ui->progressBarDetale->setValue(procent);
        }
        if (val > 0 && ui->comboDetaleStatus->currentText() == Status::Planowane) {
            ui->comboDetaleStatus->setCurrentText(Status::WTrakcie);
        }
    });

    // Zmiana wartości docelowej - zaktualizuj maksimum spina aktualnego i pasek postępu.
    connect(ui->spinDetaleDocelowy, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int maxVal) {
        ui->spinDetaleAktualny->setMaximum(maxVal);
        int val = ui->spinDetaleAktualny->value();
        if (maxVal > 0) {
            int procent = (static_cast<double>(val) / maxVal) * 100;
            ui->progressBarDetale->setValue(procent);
        }
    });

    // Kliknięcie węzła w drzewie historii - pokaż szczegóły wybranego elementu.
    connect(ui->treeHistoria, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int) {
        obsluzKlikniecieHistorii(item);
    });

    // Podpięcie pozostałych przycisków do ich slotów.
    connect(ui->btnDetaleZapisz, &QPushButton::clicked, this, &DetailsWidget::obsluzZapisz);
    connect(ui->btnNowePodejscie, &QPushButton::clicked, this, &DetailsWidget::obsluzNowePodejscie);
    connect(ui->btnCofnijZakonczenie, &QPushButton::clicked, this, &DetailsWidget::obsluzCofnijZakonczenie);
    connect(ui->btnZakonczPodejscie, &QPushButton::clicked, this, &DetailsWidget::obsluzZakonczPodejscie);
    connect(ui->btnEdytujZaznaczone, &QPushButton::clicked, this, &DetailsWidget::obsluzEdytujZaznaczone);
    connect(ui->btnUsunZaznaczone, &QPushButton::clicked, this, &DetailsWidget::obsluzUsunZaznaczone);
    connect(ui->btnUlubione, &QPushButton::clicked, this, &DetailsWidget::obsluzUlubione);
    connect(ui->btnSzybkaSesja, &QPushButton::clicked, this, &DetailsWidget::obsluzSzybkaSesja);

    // Ustaw początkowy stan przycisków.
    aktualizujStanPrzyciskowHistorii();
}

DetailsWidget::~DetailsWidget()
{
    delete ui;
}

// Ładuje dane wskazanego medium i ustawia cały widok szczegółów — układ, przyciski, liczniki.
void DetailsWidget::ustawMedium(int idMedium) {
    // Nowe medium - wróć na zakładkę 0. To samo medium (odświeżenie po zapisie) - zostań gdzie jesteś.
    int zachowanaZakladka = (idMedium == aktualneIdMedium) ? ui->tabWidget->currentIndex() : 0;
    aktualneIdMedium = idMedium;
    ui->tabWidget->setCurrentIndex(zachowanaZakladka);

    const auto lista = appController.pobierzWszystkieMultimedia();
    for (const auto& medium : lista) {
        if (medium->id != idMedium) continue;

        //   Nagłówek: tytuł, platforma, przycisk ulubionych
        ui->lblDetaleTytul->setText(medium->tytul);

        const auto platformy = appController.pobierzPlatformy();
        QString nazwaPlat = "Nieznana platforma";
        for (const auto& p : platformy) {
            if (p.first == medium->idPlatformy) { nazwaPlat = p.second; break; }
        }
        // Podpis pod tytułem łączy platformę, rok wydania i twórcę — puste człony pomijamy.
        QString podtytul = nazwaPlat;
        if (medium->rokWydania > 0)
            podtytul += QString("  ·  %1").arg(medium->rokWydania);
        if (!medium->tworcy.trimmed().isEmpty())
            podtytul += QString("  ·  %1").arg(medium->tworcy.trimmed());
        ui->labNazwaPlatformy->setText(podtytul);
        odswiezPrzyciskUlubione(medium->ulubione);

        // Ukończone/Porzucone mają zupełnie inny układ niż aktywne — różne przyciski, brak kontrolek postępu.
        bool czyZakonczone = (medium->status == Status::Ukonczone || medium->status == Status::Porzucone);

        if (czyZakonczone) {
            //   Widok zakończonego podejścia
            ui->lblWielkiStatus->setText(medium->status.toUpper());
            ui->lblWielkiStatus->show();

            // Pokaż recenzję z ostatniego podejścia — jeśli istnieje tekst lub ocena.
            auto historia = appController.pobierzHistorie(idMedium);
            if (!historia.isEmpty() && (!historia.first().recenzja.isEmpty() || historia.first().ocena > 0)) {
                auto p = historia.first();

                QString tekstDoPokazania = p.recenzja;
                if (tekstDoPokazania.trimmed().isEmpty()) {
                    tekstDoPokazania = "Brak recenzji tekstowej.";
                }

                // Widget tworzony przy pierwszym użyciu — nie zajmuje miejsca gdy nie ma recenzji.
                QTextBrowser* textRecenzja = ui->tab->findChild<QTextBrowser*>("dynamicznaRecenzja");
                if (!textRecenzja) {
                    textRecenzja = new QTextBrowser(ui->tab);
                    textRecenzja->setObjectName("dynamicznaRecenzja");
                    textRecenzja->setReadOnly(true);
                    textRecenzja->setMaximumHeight(120);
                    textRecenzja->setStyleSheet("background: #1a1c1e; padding: 10px; border-radius: 8px; color: #d1d1d1; border-left: 4px solid #2d89ef; font-style: italic;");
                    // Wiersz 7 w gridLayout — pod etykietą statusu, powyżej spacera.
                    ui->gridLayout->addWidget(textRecenzja, 7, 0);
                }
                textRecenzja->setHtml(QString("%1").arg(tekstDoPokazania));
                textRecenzja->show();
            } else {
                if (QTextBrowser* r = ui->tab->findChild<QTextBrowser*>("dynamicznaRecenzja")) r->hide();
            }

            // Tylko "Nowe podejście" i "Cofnij zakończenie" mają sens po zakończeniu.
            ui->btnNowePodejscie->show();
            ui->btnCofnijZakonczenie->show();

            ui->frameKontrolkiPostepu->hide();
            ui->progressBarDetale->hide();
            ui->comboDetaleStatus->hide();
            ui->label_2->hide();
            ui->btnSzybkaSesja->hide();
            ui->btnZakonczPodejscie->hide();
            ui->btnDetaleZapisz->hide();
            ui->lblLicznikiZaniedbania->hide();

            // Ocena wyświetlana tylko do odczytu — bez strzałek i obramowania, jak etykieta.
            ui->lblDetaleOcena->setReadOnly(true);
            ui->lblDetaleOcena->setButtonSymbols(QAbstractSpinBox::NoButtons);
            ui->lblDetaleOcena->setStyleSheet("background: transparent; border: none; color: white; font-weight: bold; font-size: 14px;");
        } else {
            //   Widok aktywnego podejścia
            ui->lblWielkiStatus->hide();
            if (QTextBrowser* r = ui->tab->findChild<QTextBrowser*>("dynamicznaRecenzja")) r->hide();

            ui->btnNowePodejscie->hide();
            ui->btnCofnijZakonczenie->hide();

            ui->frameKontrolkiPostepu->show();
            ui->progressBarDetale->show();
            ui->comboDetaleStatus->show();
            ui->label_2->show();
            ui->btnSzybkaSesja->show();
            ui->btnZakonczPodejscie->show();
            ui->btnDetaleZapisz->show();
            ui->lblLicznikiZaniedbania->show();

            // Combo statusów bez "Ukończone"/"Porzucone" — te stany ustawia się przez dedykowane przyciski.
            ui->comboDetaleStatus->clear();
            ui->comboDetaleStatus->addItems(Status::edytowalne());
            ui->comboDetaleStatus->setCurrentText(medium->status);

            // Ocena edytowalna ze strzałkami.
            ui->lblDetaleOcena->setReadOnly(false);
            ui->lblDetaleOcena->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
            ui->lblDetaleOcena->setStyleSheet("");
        }

        //   Dane postępu — wspólne dla obu widoków
        int procent = static_cast<int>(medium->postep.pobierzProcent());
        ui->progressBarDetale->setValue(procent);

        // blockSignals zapobiega wywołaniu slotu zapisu podczas programowego ustawienia wartości.
        ui->lblDetaleOcena->blockSignals(true);
        ui->lblDetaleOcena->setValue(medium->ocena);
        ui->lblDetaleOcena->blockSignals(false);

        ui->lblDetaleOcena->show();
        ui->label_4->show();

        ui->spinDetaleDocelowy->setValue(medium->postep.docelowa);
        ui->spinDetaleAktualny->setMaximum(medium->postep.docelowa);
        ui->spinDetaleAktualny->setValue(medium->postep.aktualna);
        ui->lblDetaleJednostka->setText(medium->postep.jednostka);

        // Policz łączny czas ze wszystkich sesji we wszystkich podejściach.
        const auto historia = appController.pobierzHistorie(idMedium);
        QDateTime dataStartuPodejscia;
        int sumaSekundCalkowita = 0;
        if (!historia.isEmpty()) {
            dataStartuPodejscia = historia.first().data_rozpoczecia;
            for (const auto& p : historia) {
                for (const auto& s : p.sesje) {
                    sumaSekundCalkowita += s.sekundy;
                }
            }
        }
        ui->lblDetaleCzasCalkowity->setText(QString("Łączny czas: %1h %2m").arg(sumaSekundCalkowita / 3600).arg((sumaSekundCalkowita % 3600) / 60));

        // Liczniki zaniedbania — tylko dla aktywnych podejść
        if (!czyZakonczone) {
            QDateTime teraz = QDateTime::currentDateTime();
            QString tooltipDaty = QString("Dodano: %1\nOstatnia aktywność: %2").arg(medium->dataDodania.toString("dd.MM.yyyy"), medium->dataOstatniejAktywnosci.toString("dd.MM.yyyy HH:mm"));
            QString tekstLicznikow = QString("W bibliotece od: <b>%1 dni</b>").arg(medium->dataDodania.daysTo(teraz));

            if (medium->status == Status::WTrakcie && dataStartuPodejscia.isValid()) {
                int dniOdStartu = dataStartuPodejscia.daysTo(teraz);
                int dniBezAkcji = medium->dataOstatniejAktywnosci.daysTo(teraz);
                // Czerwony kolor po 30 dniach bez żadnej sesji.
                QString kolorBrakPostepu = (dniBezAkcji > 30) ? "#e74c3c" : palette().color(QPalette::WindowText).name();
                tekstLicznikow += QString(" | Podejście aktywne od: <b>%1 dni</b> | Brak postępu od: <b style='color:%2;'>%3 dni</b>").arg(dniOdStartu).arg(kolorBrakPostepu).arg(dniBezAkcji);
                tooltipDaty += QString("\nStart podejścia: %1").arg(dataStartuPodejscia.toString("dd.MM.yyyy HH:mm"));
            } else if (medium->status == Status::Wstrzymane) {
                tekstLicznikow += QString(" | Wstrzymane od: <b>%1 dni</b>").arg(medium->dataOstatniejAktywnosci.daysTo(teraz));
            }
            ui->lblLicznikiZaniedbania->setToolTip(tooltipDaty);
            ui->lblLicznikiZaniedbania->setText(tekstLicznikow);
        }

        break;
    }
    odswiezHistorie(idMedium);
}

// Zapisuje zmiany postępu, statusu i oceny aktualnego medium do bazy.
void DetailsWidget::obsluzZapisz() {
    if (aktualneIdMedium == -1) return;

    QString nowyStatus = ui->comboDetaleStatus->currentText();
    int nowaAktualna = ui->spinDetaleAktualny->value();
    int nowaDocelowa = ui->spinDetaleDocelowy->value();
    int ocena = ui->lblDetaleOcena->value();

    // Jeśli aktualna == docelowa, a użytkownik nie ustawił ręcznie "Ukończone",
    // pytamy czy chce automatycznie zamknąć medium.
    if (nowyStatus != Status::Ukonczone && appController.czyOsiagnietoCel(nowaAktualna, nowaDocelowa)) {
        auto odpowiedz = QMessageBox::question(this, "Automatyczne ukończenie",
                                               "Wartość aktualna zrównała się z docelową. Po zapisaniu pozycja automatycznie zmieni status na 'Ukończone'.\nKontynuować?",
                                               QMessageBox::Yes | QMessageBox::No);
        if (odpowiedz == QMessageBox::No) return;
        nowyStatus = Status::Ukonczone;
    }

    if (appController.aktualizujPostep(aktualneIdMedium, nowyStatus, nowaAktualna, nowaDocelowa, ocena)) {
        ustawMedium(aktualneIdMedium);   // odśwież widok — może zmienił się układ przycisków (np. przejście do widoku zakończonego)
        emit daneZaktualizowane();       // powiadom resztę aplikacji (np. listę mediów) że dane się zmieniły
        QMessageBox::information(this, "Sukces", "Zapisano zmiany!");
    }
}

// Przebudowuje drzewo historii od zera — podejścia jako węzły-rodzice, sesje jako dzieci.
void DetailsWidget::odswiezHistorie(int idMedium) {
    // Czyścimy całe drzewo i panel szczegółów przed ponownym wypełnieniem.
    ui->treeHistoria->clear();
    ui->txtSzczegolyWpisu->clear();

    const auto historia = appController.pobierzHistorie(idMedium);

    for (const auto& p : historia) {
        // Każde podejście to węzeł-rodzic dodawany bezpośrednio do korzenia drzewa.
        QTreeWidgetItem *wezelPodejscia = new QTreeWidgetItem(ui->treeHistoria);
        wezelPodejscia->setText(0, QString("Podejście #%1 (%2)").arg(p.numer).arg(p.status));
        wezelPodejscia->setData(0, ROLA_TYP, static_cast<int>(TypHistoriiElementu::Podejscie));
        wezelPodejscia->setData(0, ROLA_ID, p.id);

        wezelPodejscia->setText(2, QString("%1/%2").arg(p.aktualna).arg(p.docelowa));

        // Data startu ustawiana raz na poziomie podejścia — sesje jej nie nadpisują.
        if (p.data_rozpoczecia.isValid()) {
            wezelPodejscia->setText(1, p.data_rozpoczecia.toString("dd.MM.yyyy HH:mm"));
        } else {
            wezelPodejscia->setText(1, "Nie rozpoczęto");
        }

        int sumaSekund = 0;
        int iloscSesji = p.sesje.size();

        for (int i = 0; i < iloscSesji; ++i) {
            const auto& s = p.sesje[i];
            sumaSekund += s.sekundy;

            // Sesja to węzeł-dziecko podpięty pod węzeł podejścia.
            QTreeWidgetItem *wezelSesji = new QTreeWidgetItem(wezelPodejscia);
            // Numerowanie odwrócone: ostatnia w liście sesja dostaje numer 1.
            wezelSesji->setText(0, QString("Sesja #%1").arg(iloscSesji - i));
            wezelSesji->setData(0, ROLA_TYP, static_cast<int>(TypHistoriiElementu::Sesja));
            wezelSesji->setData(0, ROLA_ID, s.id);
            wezelSesji->setText(1, s.data.toString("dd.MM.yyyy HH:mm"));
            wezelSesji->setText(2, QString("+%1").arg(s.przyrost));

            int h = s.sekundy / 3600;
            int m = (s.sekundy % 3600) / 60;
            wezelSesji->setText(3, QString("%1h %2m").arg(h).arg(m));

            // HTML ze szczegółami sesji — wyświetlany w panelu na dole po kliknięciu węzła.
            QString infoSesja = QString("<b>Data:</b> %1<br><b>Przyrost:</b> %2<br><b>Czas trwania:</b> %3h %4m<br><br><b>Notatka:</b><br>%5")
                                    .arg(s.data.toString("dd.MM.yyyy HH:mm")).arg(s.przyrost).arg(h).arg(m)
                                    .arg(s.notatka.isEmpty() ? "Brak notatki dla tej sesji." : s.notatka);
            wezelSesji->setData(0, Qt::UserRole, infoSesja);
        }

        // Łączny czas podejścia wyliczony po przejściu wszystkich jego sesji.
        int sumH = sumaSekund / 3600;
        int sumM = (sumaSekund % 3600) / 60;
        wezelPodejscia->setText(3, QString("%1h %2m").arg(sumH).arg(sumM));

        // HTML ze szczegółami podejścia — analogicznie jak dla sesji, wyświetlany po kliknięciu.
        QString infoPodejscie = QString("<b>Status:</b> %1<br><b>Ocena:</b> %2/10<br><br><b>Recenzja:</b><br>%3")
                                    .arg(p.status,
                                         p.ocena > 0 ? QString::number(p.ocena) : "brak",
                                         p.recenzja.isEmpty() ? "Brak recenzji." : p.recenzja);
        wezelPodejscia->setData(0, Qt::UserRole, infoPodejscie);
    }
    ui->treeHistoria->expandAll();
    aktualizujStanPrzyciskowHistorii();
}

// Wyświetla szczegóły klikniętego węzła drzewa w panelu na dole
void DetailsWidget::obsluzKlikniecieHistorii(QTreeWidgetItem *item) {
    if (!item) {
        // Kliknięcie w puste miejsce — wyczyść panel szczegółów i zresetuj przyciski.
        ui->txtSzczegolyWpisu->clear();
        aktualizujStanPrzyciskowHistorii();
        return;
    }
    // Odczytaj HTML zapisany w węźle podczas budowania drzewa i wyświetl go w panelu szczegółów.
    ui->txtSzczegolyWpisu->setHtml(item->data(0, Qt::UserRole).toString());
    // Zaktualizuj przyciski — ich etykiety i dostępność zależą od typu klikniętego węzła.
    aktualizujStanPrzyciskowHistorii();
}

// Ustawia etykiety i dostępność przycisków Edytuj/Usuń zależnie od zaznaczonego węzła.
void DetailsWidget::aktualizujStanPrzyciskowHistorii() {
    QTreeWidgetItem* item = ui->treeHistoria->currentItem();

    // Nic nie jest zaznaczone — przyciski nieaktywne z generycznym tekstem.
    if (!item) {
        ui->btnEdytujZaznaczone->setEnabled(false);
        ui->btnUsunZaznaczone->setEnabled(false);
        ui->btnEdytujZaznaczone->setText("Edytuj");
        ui->btnUsunZaznaczone->setText("Usuń");
        return;
    }

    // Odczytaj typ zaznaczonego węzła i dopasuj etykiety przycisków do kontekstu.
    // Dzięki temu użytkownik widzi "Usuń podejście" albo "Usuń sesję", a nie jedno ogólne "Usuń".
    const auto typ = static_cast<TypHistoriiElementu>(item->data(0, ROLA_TYP).toInt());
    if (typ == TypHistoriiElementu::Podejscie) {
        ui->btnEdytujZaznaczone->setEnabled(true);
        ui->btnEdytujZaznaczone->setText("Edytuj recenzję/ocenę");
        ui->btnUsunZaznaczone->setEnabled(true);
        ui->btnUsunZaznaczone->setText("Usuń podejście");
    } else if (typ == TypHistoriiElementu::Sesja) {
        ui->btnEdytujZaznaczone->setEnabled(true);
        ui->btnEdytujZaznaczone->setText("Edytuj sesję");
        ui->btnUsunZaznaczone->setEnabled(true);
        ui->btnUsunZaznaczone->setText("Usuń sesję");
    } else {
        // Nieznany typ — defensywnie wyłącz przyciski.
        ui->btnEdytujZaznaczone->setEnabled(false);
        ui->btnUsunZaznaczone->setEnabled(false);
    }
}

// Wyświetla dialog dodawania lub edycji sesji. Zwraca true jeśli użytkownik zatwierdził,
// false jeśli anulował lub dane były puste.
bool DetailsWidget::pokazDialogSesji(int& przyrost, int& sekundy, QString& notatka, const QString& tytul, bool edycja) {
    QDialog dialog(this);
    dialog.setWindowTitle(tytul);
    dialog.setMinimumWidth(380);
    auto* glownyUklad = new QVBoxLayout(&dialog);

    // Sekcja postępu — spinner z zakresem ujemnym bo można cofnąć postęp (np. korekta błędu).
    auto* groupPrzyrost = new QGroupBox("Postęp", &dialog);
    auto* formPrzyrost = new QFormLayout(groupPrzyrost);
    auto* spinPrzyrost = new QSpinBox(&dialog);
    spinPrzyrost->setRange(-9999, 9999);
    spinPrzyrost->setValue(przyrost);

    // Nazwa jednostki pobierana z aktywnego widoku detali, żeby etykieta była konkretna (np. "strony").
    QString nazwaJednostki = ui->lblDetaleJednostka->text();
    if(nazwaJednostki.isEmpty()) nazwaJednostki = "jednostek";

    formPrzyrost->addRow(QString("Zdobyte (%1):").arg(nazwaJednostki), spinPrzyrost);
    glownyUklad->addWidget(groupPrzyrost);

    // Sekcja czasu — stoper i ręczna korekta działają niezależnie, ale są zsynchronizowane.
    auto* groupCzas = new QGroupBox("Czas sesji", &dialog);
    auto* layoutCzas = new QVBoxLayout(groupCzas);

    // Consolas bo to czcionka monospace — cyfry mają stałą szerokość i wyświetlacz nie skacze.
    auto* lblStoper = new QLabel("00:00:00", &dialog);
    lblStoper->setFont(QFont("Consolas", 28, QFont::Bold));
    lblStoper->setAlignment(Qt::AlignCenter);
    lblStoper->setStyleSheet("color: #2d89ef; margin: 10px 0;");

    auto* layoutPrzyciskiStopera = new QHBoxLayout();
    auto* btnStart = new QPushButton(QIcon(":/icons/play.svg"), "Start", &dialog);
    auto* btnReset = new QPushButton(QIcon(":/icons/reset.svg"), "Reset", &dialog);
    btnStart->setIconSize(QSize(16, 16));
    btnReset->setIconSize(QSize(16, 16));
    btnStart->setMinimumHeight(35);
    btnReset->setMinimumHeight(35);

    layoutPrzyciskiStopera->addWidget(btnStart);
    layoutPrzyciskiStopera->addWidget(btnReset);

    layoutCzas->addWidget(lblStoper);
    layoutCzas->addLayout(layoutPrzyciskiStopera);

    // Ręczna korekta czasu — spinnery h/m/s zsynchronizowane ze stoperem w obie strony.
    auto* layoutReczny = new QHBoxLayout();
    auto* spinH = new QSpinBox(&dialog); spinH->setRange(0, 999); spinH->setSuffix(" h");
    auto* spinM = new QSpinBox(&dialog); spinM->setRange(0, 59);  spinM->setSuffix(" m");
    auto* spinS = new QSpinBox(&dialog); spinS->setRange(0, 59);  spinS->setSuffix(" s");

    layoutReczny->addWidget(new QLabel("Korekta ręczna:", &dialog));
    layoutReczny->addStretch();
    layoutReczny->addWidget(spinH);
    layoutReczny->addWidget(spinM);
    layoutReczny->addWidget(spinS);
    layoutCzas->addLayout(layoutReczny);

    glownyUklad->addWidget(groupCzas);

    // Sekcja notatki — opcjonalna, wstępnie wypełniona przy edycji.
    auto* groupNotatka = new QGroupBox("Notatka", &dialog);
    auto* layoutNotatka = new QVBoxLayout(groupNotatka);
    auto* txtNotatka = new QTextEdit(&dialog);
    txtNotatka->setPlainText(notatka);
    txtNotatka->setPlaceholderText("Coś ciekawego się wydarzyło? (Opcjonalnie)");
    txtNotatka->setMinimumHeight(80);
    layoutNotatka->addWidget(txtNotatka);
    glownyUklad->addWidget(groupNotatka);

    auto* przyciski = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    glownyUklad->addWidget(przyciski);

    // Wewnętrzny licznik sekund — inicjalizowany poprzednią wartością przy edycji, zerem przy nowej sesji.
    int lacznieSekund = sekundy;

    // Lambda aktualizująca jednocześnie wyświetlacz stopera i trzy spinnery.
    // blockSignals zapobiega nieskończonej pętli: zmiana spinnera wywołałaby recznaZmiana,
    // który nadpisałby lacznieSekund i ponownie wywołał odswiezWyswietlacz.
    auto odswiezWyswietlacz = [&]() {
        int h = lacznieSekund / 3600;
        int m = (lacznieSekund % 3600) / 60;
        int s = lacznieSekund % 60;

        spinH->blockSignals(true); spinM->blockSignals(true); spinS->blockSignals(true);
        spinH->setValue(h); spinM->setValue(m); spinS->setValue(s);
        spinH->blockSignals(false); spinM->blockSignals(false); spinS->blockSignals(false);

        lblStoper->setText(QString("%1:%2:%3")
                               .arg(h, 2, 10, QChar('0'))
                               .arg(m, 2, 10, QChar('0'))
                               .arg(s, 2, 10, QChar('0')));
    };
    // Wywołanie inicjalizujące — przy edycji od razu pokazuje poprzedni czas.
    odswiezWyswietlacz();

    // Timer tykający co sekundę — przy każdym tyknięciu zwiększa licznik i odświeża widok.
    QTimer* czasomierz = new QTimer(&dialog);
    connect(czasomierz, &QTimer::timeout, [&]() {
        lacznieSekund++;
        odswiezWyswietlacz();
    });

    // Jeden przycisk pełni dwie role: Start i Pauza. Czerwone tło sygnalizuje że czas leci.
    connect(btnStart, &QPushButton::clicked, [&]() {
        if (czasomierz->isActive()) {
            czasomierz->stop();
            btnStart->setIcon(QIcon(":/icons/play.svg"));
            btnStart->setText("Start");
            btnStart->setStyleSheet("");
        } else {
            czasomierz->start(1000);
            btnStart->setIcon(QIcon(":/icons/pause.svg"));
            btnStart->setText("Pauza");
            btnStart->setStyleSheet("background-color: #c0392b; color: white;");
        }
    });

    // Reset zeruje licznik. Jeśli stoper był zatrzymany, przywraca tekst przycisku do "Start".
    connect(btnReset, &QPushButton::clicked, [&]() {
        lacznieSekund = 0;
        odswiezWyswietlacz();
        if (!czasomierz->isActive()) {
            btnStart->setIcon(QIcon(":/icons/play.svg"));
            btnStart->setText("Start");
        }
    });

    // Ręczna zmiana któregokolwiek spinnera przelicza lacznieSekund i odświeża wyświetlacz.
    auto recznaZmiana = [&]() {
        lacznieSekund = spinH->value() * 3600 + spinM->value() * 60 + spinS->value();
        odswiezWyswietlacz();
    };
    connect(spinH, QOverload<int>::of(&QSpinBox::valueChanged), this, recznaZmiana);
    connect(spinM, QOverload<int>::of(&QSpinBox::valueChanged), this, recznaZmiana);
    connect(spinS, QOverload<int>::of(&QSpinBox::valueChanged), this, recznaZmiana);

    connect(przyciski, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(przyciski, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // exec() blokuje do momentu zamknięcia dialogu.
    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    // Zapisz wyniki do zmiennych wyjściowych — tędy dane wracają do wywołującego kodu.
    przyrost = spinPrzyrost->value();
    sekundy = lacznieSekund;
    notatka = txtNotatka->toPlainText().trimmed();

    // Walidacja tylko dla nowych sesji — przy edycji puste dane są dopuszczalne.
    if (!edycja && przyrost == 0 && sekundy == 0 && notatka.isEmpty()) {
        QMessageBox::warning(this, "Puste dane", "Sesja nie może być całkowicie pusta.\nMusisz spędzić chociaż sekundę, zrobić postęp lub dodać notatkę.");
        return false;
    }
    return true;
}

// Otwiera dialog edycji zaznaczonego węzła — inny formularz dla podejścia, inny dla sesji.
void DetailsWidget::obsluzEdytujZaznaczone() {
    QTreeWidgetItem* item = ui->treeHistoria->currentItem();
    if (!item) return;

    // Odczytaj typ węzła — edycja podejścia i sesji to dwa zupełnie różne dialogi.
    const auto typ = static_cast<TypHistoriiElementu>(item->data(0, ROLA_TYP).toInt());

    if (typ == TypHistoriiElementu::Podejscie) {
        const int idPodejscia = item->data(0, ROLA_ID).toInt();
        const auto historia = appController.pobierzHistorie(aktualneIdMedium);

        for (const auto& p : historia) {
            if (p.id != idPodejscia) continue;

            // Edycja podejścia pozwala zmienić tylko ocenę i recenzję — status i postęp
            // są celowo pominięte, żeby nie nadpisać danych zmienianych przez inne przyciski.
            QDialog d(this);
            d.setWindowTitle("Edytuj podsumowanie");
            QFormLayout f(&d);
            QSpinBox s; s.setRange(0, 10); s.setValue(p.ocena);
            QTextEdit t; t.setPlainText(p.recenzja);
            f.addRow("Ocena:", &s);
            f.addRow("Recenzja:", &t);
            QDialogButtonBox b(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
            f.addRow(&b);
            connect(&b, &QDialogButtonBox::accepted, &d, &QDialog::accept);
            connect(&b, &QDialogButtonBox::rejected, &d, &QDialog::reject);

            if (d.exec() == QDialog::Accepted) {
                // Przekazujemy oryginalne status/aktualna/docelowa — zmieniamy tylko ocenę i recenzję.
                appController.aktualizujPodejscie(idPodejscia, p.status, p.aktualna, p.docelowa, s.value(), t.toPlainText().trimmed());
                ustawMedium(aktualneIdMedium);
                emit daneZaktualizowane();
            }
            return;
        }
    } else if (typ == TypHistoriiElementu::Sesja) {
        const int idSesji = item->data(0, ROLA_ID).toInt();
        const auto historia = appController.pobierzHistorie(aktualneIdMedium);

        for (const auto& p : historia) {
            for (const auto& s : p.sesje) {
                if (s.id != idSesji) continue;

                // Wstępnie wypełniamy danymi istniejącej sesji — użytkownik widzi co edytuje.
                int przyrost = s.przyrost;
                int sekundy = s.sekundy;
                QString notatka = s.notatka;

                // Otwórz dialog w trybie edycji (true = walidacja pustych danych wyłączona).
                // Jeśli użytkownik kliknął Anuluj, funkcja zwraca false — przerywamy bez zapisu.
                if (!pokazDialogSesji(przyrost, sekundy, notatka, "Edytuj sesję", true)) {
                    return;
                }
                if (!appController.aktualizujSesje(idSesji, przyrost, sekundy, notatka)) {
                    QMessageBox::warning(this, "Błąd", "Nie udało się zaktualizować sesji.");
                    return;
                }
                ustawMedium(aktualneIdMedium);
                emit daneZaktualizowane();
                return;
            }
        }
    }
}

// Usuwa zaznaczony węzeł (podejście lub sesję) po potwierdzeniu przez użytkownika.
void DetailsWidget::obsluzUsunZaznaczone() {
    QTreeWidgetItem* item = ui->treeHistoria->currentItem();
    if (!item) return;

    const auto typ = static_cast<TypHistoriiElementu>(item->data(0, ROLA_TYP).toInt());

    // Potwierdzenie usunięcia — operacja nieodwracalna.
    QString etykieta = (typ == TypHistoriiElementu::Podejscie) ? "podejście" : "sesję";
    auto odpowiedz = QMessageBox::question(
        this,
        "Potwierdzenie usunięcia",
        QString("Czy na pewno chcesz usunąć to %1? Tej operacji nie można cofnąć.").arg(etykieta),
        QMessageBox::Yes | QMessageBox::No
    );
    if (odpowiedz != QMessageBox::Yes) {
        return;
    }

    // Wywołaj odpowiednią metodę kontrolera zależnie od typu węzła.
    bool ok = false;
    if (typ == TypHistoriiElementu::Podejscie) {
        ok = appController.usunPodejscie(item->data(0, ROLA_ID).toInt());
    } else if (typ == TypHistoriiElementu::Sesja) {
        ok = appController.usunSesje(item->data(0, ROLA_ID).toInt());
    }

    if (!ok) {
        QMessageBox::warning(this, "Błąd", "Nie udało się usunąć zaznaczonego elementu.");
        return;
    }

    ustawMedium(aktualneIdMedium);
    emit daneZaktualizowane();
}

// Aktualizuje wygląd przycisku ulubionych — ikonę i tooltip zależnie od stanu.
// Wywoływana zarówno przy ładowaniu medium jak i po kliknięciu przycisku.
void DetailsWidget::odswiezPrzyciskUlubione(bool czyUlubione) {
    ui->btnUlubione->setIcon(QIcon(czyUlubione ? ":/icons/heart-filled.svg" : ":/icons/heart-outline.svg"));
    ui->btnUlubione->setIconSize(QSize(22, 22));
    ui->btnUlubione->setText("");
    ui->btnUlubione->setToolTip(czyUlubione ? "Usuń z ulubionych" : "Dodaj do ulubionych");
    ui->btnUlubione->setStyleSheet("QPushButton { border: none; background: transparent; }");
    // Stan przechowywany jako właściwość przycisku — odczytywany przy kliknięciu.
    ui->btnUlubione->setProperty("czyUlubione", czyUlubione);
}

// Przełącza status ulubionych aktualnego medium i odświeża przycisk.
void DetailsWidget::obsluzUlubione() {
    if (aktualneIdMedium <= 0) return;

    // Stan odczytywany z właściwości ustawionej w odswiezPrzyciskUlubione.
    bool obecnyStan = ui->btnUlubione->property("czyUlubione").toBool();
    bool nowyStan = !obecnyStan;

    if (!appController.ustawUlubione(aktualneIdMedium, nowyStan)) {
        QMessageBox::warning(this, "Błąd", "Nie udało się zaktualizować statusu ulubionych.");
        return;
    }

    odswiezPrzyciskUlubione(nowyStan);
}

// Dodaje nową sesję do aktualnego (najnowszego) podejścia.
void DetailsWidget::obsluzSzybkaSesja() {
    const auto historia = appController.pobierzHistorie(aktualneIdMedium);
    if (historia.isEmpty()) return;

    // Sesja trafia zawsze do pierwszego podejścia na liście — to jest aktywne podejście.
    int idPodejscia = historia.first().id;
    int przyrost = 0; int sekundy = 0; QString notatka;

    // Tryb nowej sesji (false)
    if (pokazDialogSesji(przyrost, sekundy, notatka, "Szybkie dodawanie aktywności", false)) {
        if (appController.dodajSesje(idPodejscia, przyrost, sekundy, notatka)) {
            ustawMedium(aktualneIdMedium);
            emit daneZaktualizowane();
        }
    }
}

// Tworzy nowe podejście dla aktualnego medium po tym jak poprzednie zostało zakończone.
void DetailsWidget::obsluzNowePodejscie() {
    if (aktualneIdMedium == -1) return;

    // Domyślny cel to poprzedni cel — użytkownik może go zmienić w dialogu.
    bool ok;
    int nowyCel = QInputDialog::getInt(
        this, "Nowe podejście",
        "Świetnie, że dajesz temu tytułowi nową szansę!\nPodaj cel dla nowego podejścia:",
        ui->spinDetaleDocelowy->value(), 1, 9999, 1, &ok
    );

    if (ok) {
        if (appController.dodajPodejscie(aktualneIdMedium, Status::WTrakcie, nowyCel)) {
            ustawMedium(aktualneIdMedium);
            emit daneZaktualizowane();
        }
    }
}

// Przywraca ostatnie podejście ze statusu Ukończone/Porzucone z powrotem do "W trakcie".
void DetailsWidget::obsluzCofnijZakonczenie() {
    const auto historia = appController.pobierzHistorie(aktualneIdMedium);
    if (historia.isEmpty()) return;

    int idPodejscia = historia.first().id;

    if (QMessageBox::question(this, "Cofnij zakończenie",
                              "Czy na pewno chcesz cofnąć zakończenie i przywrócić to podejście do statusu 'W trakcie'?",
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        // Zmieniamy tylko status — postęp, ocena i recenzja zostają bez zmian.
        if (appController.aktualizujPodejscie(idPodejscia, Status::WTrakcie,
                historia.first().aktualna, historia.first().docelowa,
                historia.first().ocena, historia.first().recenzja)) {
            ustawMedium(aktualneIdMedium);
            emit daneZaktualizowane();
        }
    }
}

// Kończy aktywne podejście — otwiera dialog z podsumowaniem (ocena, recenzja, status końcowy).
void DetailsWidget::obsluzZakonczPodejscie() {
    int akt = ui->spinDetaleAktualny->value();
    int doc = ui->spinDetaleDocelowy->value();
    // Cel osiągnięty jeśli aktualna >= docelowa i docelowa jest sensowna (> 0).
    bool osiagnietoCel = (akt >= doc && doc > 0);

    const auto historia = appController.pobierzHistorie(aktualneIdMedium);
    if (historia.isEmpty()) return;
    // Wstępnie wypełniamy recenzją z poprzedniego zapisu — użytkownik może ją zmienić.
    QString istniejącaRecenzja = historia.first().recenzja;

    QDialog dialog(this);
    dialog.setWindowTitle("Podsumowanie podejścia");
    dialog.resize(450, 350);
    QVBoxLayout layout(&dialog);

    // Komunikat różni się zależnie od tego czy cel został osiągnięty.
    QLabel lblKomunikat;
    if (osiagnietoCel) {
        lblKomunikat.setText("<span style='font-size: 14px; color: #27ae60;'><b>Gratulacje!</b> Osiągnąłeś założony cel.</span><br>Podsumuj swoje doświadczenie:");
    } else {
        lblKomunikat.setText("<span style='font-size: 14px; color: #e74c3c;'><b>Nie osiągnąłeś celu.</b></span><br>Oznacz podejście jako porzucone lub wymuś ukończenie:");
    }
    lblKomunikat.setTextFormat(Qt::RichText);
    layout.addWidget(&lblKomunikat);

    QFormLayout form;

    // Status domyślnie dopasowany do wyniku — Ukończone jeśli cel osiągnięty, Porzucone jeśli nie.
    QComboBox comboStatus;
    comboStatus.addItems(Status::terminalne());
    comboStatus.setCurrentText(osiagnietoCel ? Status::Ukonczone : Status::Porzucone);

    QSpinBox spinOcena;
    spinOcena.setRange(0, 10);
    spinOcena.setSpecialValueText("Brak oceny"); // 0 zapisuje się w bazie jako brak oceny
    spinOcena.setValue(ui->lblDetaleOcena->value());

    QTextEdit txtRecenzja;
    txtRecenzja.setPlainText(istniejącaRecenzja);
    txtRecenzja.setPlaceholderText("Dopisz coś lub zmień podsumowanie...");

    form.addRow("Status końcowy:", &comboStatus);
    form.addRow("Ostateczna ocena (1-10):", &spinOcena);
    form.addRow("Recenzja:", &txtRecenzja);
    layout.addLayout(&form);

    QDialogButtonBox przyciski(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    przyciski.button(QDialogButtonBox::Save)->setText("Zapieczętuj podejście");
    layout.addWidget(&przyciski);

    connect(&przyciski, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&przyciski, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        int idPodejscia = historia.first().id;
        QString finalnyStatus = comboStatus.currentText();
        int finalnaOcena = spinOcena.value();
        QString finalnaRecenzja = txtRecenzja.toPlainText().trimmed();

        if (appController.aktualizujPodejscie(idPodejscia, finalnyStatus, akt, doc, finalnaOcena, finalnaRecenzja)) {
            ustawMedium(aktualneIdMedium);
            emit daneZaktualizowane();
        }
    }
}
