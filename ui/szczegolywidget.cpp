#include "szczegolywidget.h"
#include "ui_szczegolywidget.h"
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

namespace {
constexpr int ROLE_TYP = Qt::UserRole + 1;
constexpr int ROLE_ID = Qt::UserRole + 2;
constexpr int ROLE_ID_PODEJSCIA = Qt::UserRole + 3;
}

SzczegolyWidget::SzczegolyWidget(AppController& controller, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SzczegolyWidget),
    appController(controller)
{
    ui->setupUi(this);

    ui->btnCofnijZakonczenie->hide();
    ui->treeHistoria->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->btnDodajPodejscie->hide();
    ui->btnDodajSesje->hide();

    if (ui->comboDetaleStatus->count() == 0) {
        ui->comboDetaleStatus->addItems(appController.pobierzDostepneStatusy());
    }

    connect(ui->spinDetaleAktualny, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        int maxVal = ui->spinDetaleDocelowy->value();
        if (maxVal > 0) {
            int procent = (static_cast<double>(val) / maxVal) * 100;
            ui->progressBarDetale->setValue(procent);
        }
        if (val > 0 && ui->comboDetaleStatus->currentText() == "Planowane") {
            ui->comboDetaleStatus->setCurrentText("W trakcie");
        }
    });

    connect(ui->spinDetaleDocelowy, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int maxVal) {
        ui->spinDetaleAktualny->setMaximum(maxVal);
        int val = ui->spinDetaleAktualny->value();
        if (maxVal > 0) {
            int procent = (static_cast<double>(val) / maxVal) * 100;
            ui->progressBarDetale->setValue(procent);
        }
    });


    connect(ui->treeHistoria, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int) {
        onHistoriaItemClicked(item);
    });

    connect(ui->btnDetaleZapisz, &QPushButton::clicked, this, &SzczegolyWidget::onBtnZapiszClicked);
    connect(ui->btnNowePodejscie, &QPushButton::clicked, this, &SzczegolyWidget::onBtnNowePodejscieClicked);
    connect(ui->btnCofnijZakonczenie, &QPushButton::clicked, this, &SzczegolyWidget::onBtnCofnijZakonczenieClicked);
    connect(ui->btnZakonczPodejscie, &QPushButton::clicked, this, &SzczegolyWidget::onBtnZakonczPodejscieClicked);
    connect(ui->btnEdytujZaznaczone, &QPushButton::clicked, this, &SzczegolyWidget::onBtnEdytujZaznaczoneClicked);
    connect(ui->btnUsunZaznaczone, &QPushButton::clicked, this, &SzczegolyWidget::onBtnUsunZaznaczoneClicked);
    connect(ui->btnUlubione, &QPushButton::clicked, this, &SzczegolyWidget::onBtnUlubioneClicked);
    connect(ui->btnSzybkaSesja, &QPushButton::clicked, this, &SzczegolyWidget::onBtnSzybkaSesjaClicked);

    aktualizujStanPrzyciskowHistorii();
}

SzczegolyWidget::~SzczegolyWidget()
{
    delete ui;
}

void SzczegolyWidget::ustawMedium(int idMedium) {
    // Przy nowym medium wracamy na zakładkę 0 (przegląd).
    // Przy odświeżaniu tego samego medium (po zapisie) zostajemy na aktywnej zakładce.
    int zachowanaZakladka = (idMedium == aktualneIdMedium) ? ui->tabWidget->currentIndex() : 0;
    aktualneIdMedium = idMedium;
    ui->tabWidget->setCurrentIndex(zachowanaZakladka);

    const auto lista = appController.pobierzWszystkieMultimedia();
    for (const auto& medium : lista) {
        if (medium->getId() == idMedium) {
            ui->lblDetaleTytul->setText(medium->getTytul());

            const auto platformy = appController.pobierzPlatformy();
            QString nazwaPlat = "Nieznana platforma";
            for (const auto& p : platformy) {
                if (p.first == medium->getIdPlatformy()) { nazwaPlat = p.second; break; }
            }
            ui->labNazwaPlatformy->setText(nazwaPlat);
            odswiezPrzyciskUlubione(medium->getCzyUlubione());

            // Widok różni się drastycznie zależnie od tego, czy podejście jest aktywne czy zakończone.
            bool czyZakonczone = (medium->getStatus() == "Ukończone" || medium->getStatus() == "Porzucone");

            if (czyZakonczone) {
                ui->lblWielkiStatus->setText(medium->getStatus().toUpper());
                ui->lblWielkiStatus->show();

                // Pokazujemy recenzję z ostatniego podejścia (jeśli istnieje).
                auto historia = appController.pobierzHistorie(idMedium);
                if (!historia.isEmpty() && (!historia.first().recenzja.isEmpty() || historia.first().ocena > 0)) {
                    auto p = historia.first();

                    QString tekstDoPokazania = p.recenzja;
                    if (tekstDoPokazania.trimmed() == "0" || tekstDoPokazania.trimmed().isEmpty()) {
                        tekstDoPokazania = "Brak recenzji tekstowej.";
                    }

                    // Widget tworzony leniwie, żeby nie zajmował miejsca kiedy nie ma recenzji.
                    QTextBrowser* textRecenzja = ui->tab->findChild<QTextBrowser*>("dynamicznaRecenzja");
                    if (!textRecenzja) {
                        textRecenzja = new QTextBrowser(ui->tab);
                        textRecenzja->setObjectName("dynamicznaRecenzja");
                        textRecenzja->setReadOnly(true);
                        textRecenzja->setMaximumHeight(120);
                        textRecenzja->setStyleSheet("background: #1a1c1e; padding: 10px; border-radius: 8px; color: #d1d1d1; border-left: 4px solid #2d89ef; font-style: italic;");

                        // Wiersz 7 w gridLayout zakładki — pod etykietą statusu, powyżej spacera.
                        ui->gridLayout->addWidget(textRecenzja, 7, 0);
                    }
                    textRecenzja->setHtml(QString("„%1”").arg(tekstDoPokazania));
                    textRecenzja->show();
                } else {
                    if (QTextBrowser* r = ui->tab->findChild<QTextBrowser*>("dynamicznaRecenzja")) r->hide();
                }

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

                // Ocena tylko do odczytu — zakończone podejście nie powinno być edytowalne z tego panelu.
                ui->lblDetaleOcena->setReadOnly(true);
                ui->lblDetaleOcena->setButtonSymbols(QAbstractSpinBox::NoButtons);
                ui->lblDetaleOcena->setStyleSheet("background: transparent; border: none; color: white; font-weight: bold; font-size: 14px;");
            } else {
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

                ui->comboDetaleStatus->clear();
                ui->comboDetaleStatus->addItems({"Planowane", "W trakcie", "Wstrzymane"});
                ui->comboDetaleStatus->setCurrentText(medium->getStatus());

                ui->lblDetaleOcena->setReadOnly(false);
                ui->lblDetaleOcena->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
                ui->lblDetaleOcena->setStyleSheet("");
            }

            int procent = static_cast<int>(medium->getPostep().getProcent());
            ui->progressBarDetale->setValue(procent);

            // Bezpieczne wstawienie oceny (bez wywoływania sygnału zapisu)
            ui->lblDetaleOcena->blockSignals(true);
            ui->lblDetaleOcena->setValue(medium->getOcena());
            ui->lblDetaleOcena->blockSignals(false);

            ui->lblDetaleOcena->show();
            ui->label_4->show();

            ui->spinDetaleDocelowy->setValue(medium->getPostep().docelowa);
            ui->spinDetaleAktualny->setMaximum(medium->getPostep().docelowa);
            ui->spinDetaleAktualny->setValue(medium->getPostep().aktualna);
            ui->lblDetaleJednostka->setText(medium->getPostep().jednostka);

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

            if (!czyZakonczone) {
                QDateTime teraz = QDateTime::currentDateTime();
                QString tekstLicznikow;
                QString tooltipDaty = QString("Dodano: %1\nOstatnia aktywność: %2").arg(medium->getDataDodania().toString("dd.MM.yyyy")).arg(medium->getDataOstatniejAktywnosci().toString("dd.MM.yyyy HH:mm"));
                int dniOdDodania = medium->getDataDodania().daysTo(teraz);
                tekstLicznikow = QString("W bibliotece od: <b>%1 dni</b>").arg(dniOdDodania);

                if (medium->getStatus() == "W trakcie" && dataStartuPodejscia.isValid()) {
                    int dniOdStartu = dataStartuPodejscia.daysTo(teraz);
                    int dniBezAkcji = medium->getDataOstatniejAktywnosci().daysTo(teraz);
                    QString kolorBrakPostepu = (dniBezAkcji > 30) ? "#e74c3c" : palette().color(QPalette::WindowText).name();
                    tekstLicznikow += QString(" | Podejście aktywne od: <b>%1 dni</b> | Brak postępu od: <b style='color:%2;'>%3 dni</b>").arg(dniOdStartu).arg(kolorBrakPostepu).arg(dniBezAkcji);
                    tooltipDaty += QString("\nStart podejścia: %1").arg(dataStartuPodejscia.toString("dd.MM.yyyy HH:mm"));
                } else if (medium->getStatus() == "Wstrzymane") {
                    tekstLicznikow += QString(" | Wstrzymane od: <b>%1 dni</b>").arg(medium->getDataOstatniejAktywnosci().daysTo(teraz));
                }
                ui->lblLicznikiZaniedbania->setToolTip(tooltipDaty);
                ui->lblLicznikiZaniedbania->setText(tekstLicznikow);
            }

            break;
        }
    }
    odswiezHistorie(idMedium);
}

void SzczegolyWidget::onBtnZapiszClicked() {
    if (aktualneIdMedium == -1) return;

    QString nowyStatus = ui->comboDetaleStatus->currentText();
    int nowaAktualna = ui->spinDetaleAktualny->value();
    int nowaDocelowa = ui->spinDetaleDocelowy->value();
    int ocena = ui->lblDetaleOcena->value();

    if (nowyStatus != "Ukończone" && appController.czyOsiagnietoCel(nowaAktualna, nowaDocelowa)) {
        auto odpowiedz = QMessageBox::question(this, "Automatyczne ukończenie",
                                               "Wartość aktualna zrównała się z docelową. Po zapisaniu pozycja automatycznie zmieni status na 'Ukończone'.\nKontynuować?",
                                               QMessageBox::Yes | QMessageBox::No);
        if (odpowiedz == QMessageBox::No) return;
        nowyStatus = "Ukończone";
    }

    if (appController.aktualizujPostep(aktualneIdMedium, nowyStatus, nowaAktualna, nowaDocelowa, ocena)) {
        ustawMedium(aktualneIdMedium);
        emit daneZaktualizowane();
        QMessageBox::information(this, "Sukces", "Zapisano zmiany!");
    }
}

void SzczegolyWidget::odswiezHistorie(int idMedium) {
    ui->treeHistoria->clear();
    ui->txtSzczegolyWpisu->clear();

    const auto historia = appController.pobierzHistorie(idMedium);

    for (const auto& p : historia) {
        QTreeWidgetItem *pNode = new QTreeWidgetItem(ui->treeHistoria);
        pNode->setText(0, QString("Podejście #%1 (%2)").arg(p.numer).arg(p.status));
        pNode->setData(0, ROLE_TYP, static_cast<int>(TypHistoriiElementu::Podejscie));
        pNode->setData(0, ROLE_ID, p.id);

        pNode->setText(2, QString("%1/%2").arg(p.aktualna).arg(p.docelowa));

        // Kolumna 1 ustawiana raz tutaj — sesje jej nie nadpisują.
        if (p.data_rozpoczecia.isValid()) {
            pNode->setText(1, p.data_rozpoczecia.toString("dd.MM.yyyy HH:mm"));
        } else {
            pNode->setText(1, "Nie rozpoczęto");
        }

        int sumaSekund = 0;
        int iloscSesji = p.sesje.size();

        for (int i = 0; i < iloscSesji; ++i) {
            const auto& s = p.sesje[i];
            sumaSekund += s.sekundy;

            QTreeWidgetItem *sNode = new QTreeWidgetItem(pNode);
            sNode->setText(0, QString("Sesja #%1").arg(iloscSesji - i));
            sNode->setData(0, ROLE_TYP, static_cast<int>(TypHistoriiElementu::Sesja));
            sNode->setData(0, ROLE_ID, s.id);
            sNode->setData(0, ROLE_ID_PODEJSCIA, p.id);
            sNode->setText(1, s.data.toString("dd.MM.yyyy HH:mm"));
            sNode->setText(2, QString("+%1").arg(s.przyrost));

            int h = s.sekundy / 3600;
            int m = (s.sekundy % 3600) / 60;
            sNode->setText(3, QString("%1h %2m").arg(h).arg(m));

            QString infoSesja = QString("<b>Data:</b> %1<br><b>Przyrost:</b> %2<br><b>Czas trwania:</b> %3h %4m<br><br><b>Notatka:</b><br>%5")
                                    .arg(s.data.toString("dd.MM.yyyy HH:mm")).arg(s.przyrost).arg(h).arg(m)
                                    .arg(s.notatka.isEmpty() ? "Brak notatki dla tej sesji." : s.notatka);
            sNode->setData(0, Qt::UserRole, infoSesja);
        }

        int sumH = sumaSekund / 3600;
        int sumM = (sumaSekund % 3600) / 60;
        pNode->setText(3, QString("%1h %2m").arg(sumH).arg(sumM));

        QString infoPodejscie = QString("<b>Status:</b> %1<br><b>Ocena:</b> %2/10<br><br><b>Recenzja:</b><br>%3")
                                    .arg(p.status).arg(p.ocena > 0 ? QString::number(p.ocena) : "brak")
                                    .arg(p.recenzja.isEmpty() ? "Brak recenzji." : p.recenzja);
        pNode->setData(0, Qt::UserRole, infoPodejscie);
    }
    ui->treeHistoria->expandAll();
    aktualizujStanPrzyciskowHistorii();
}

void SzczegolyWidget::onHistoriaItemClicked(QTreeWidgetItem *item) {
    if (!item) {
        ui->txtSzczegolyWpisu->clear();
        aktualizujStanPrzyciskowHistorii();
        return;
    }
    ui->txtSzczegolyWpisu->setHtml(item->data(0, Qt::UserRole).toString());
    aktualizujStanPrzyciskowHistorii();
}

void SzczegolyWidget::aktualizujStanPrzyciskowHistorii() {
    QTreeWidgetItem* item = ui->treeHistoria->currentItem();
    if (!item) {
        ui->btnEdytujZaznaczone->setEnabled(false);
        ui->btnUsunZaznaczone->setEnabled(false);
        ui->btnEdytujZaznaczone->setText("Edytuj");
        ui->btnUsunZaznaczone->setText("Usuń");
        return;
    }

    const auto typ = static_cast<TypHistoriiElementu>(item->data(0, ROLE_TYP).toInt());
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
        ui->btnEdytujZaznaczone->setEnabled(false);
        ui->btnUsunZaznaczone->setEnabled(false);
    }
}

bool SzczegolyWidget::pokazDialogSesji(int& przyrost, int& sekundy, QString& notatka, const QString& tytul, bool edycja) {
    QDialog dialog(this);
    dialog.setWindowTitle(tytul);
    dialog.setMinimumWidth(380);
    auto* mainLayout = new QVBoxLayout(&dialog);

    // --- Sekcja: Przyrost Jednostek ---
    auto* formPrzyrost = new QFormLayout();
    auto* spinPrzyrost = new QSpinBox(&dialog);
    spinPrzyrost->setRange(-9999, 9999);
    spinPrzyrost->setValue(przyrost);

    // Pobieramy jednostkę z widgetu detali żeby ładnie wyglądało
    QString nazwaJednostki = ui->lblDetaleJednostka->text();
    if(nazwaJednostki.isEmpty()) nazwaJednostki = "jednostek";

    formPrzyrost->addRow(QString("Zdobyte (%1):").arg(nazwaJednostki), spinPrzyrost);
    mainLayout->addLayout(formPrzyrost);

    // --- Sekcja: Czas (Stoper + Ręcznie) ---
    auto* groupCzas = new QGroupBox("Czas sesji", &dialog);
    auto* layoutCzas = new QVBoxLayout(groupCzas);

    auto* layoutStoper = new QHBoxLayout();
    auto* lblStoper = new QLabel("00:00:00", &dialog);
    lblStoper->setFont(QFont("Consolas", 28, QFont::Bold)); // Consolas — monospace, cyfry mają stałą szerokość
    lblStoper->setAlignment(Qt::AlignCenter);
    lblStoper->setStyleSheet("color: #2d89ef; margin: 10px 0;");

    auto* layoutPrzyciskiStopera = new QHBoxLayout();
    auto* btnStart = new QPushButton("▶ Start", &dialog);
    auto* btnReset = new QPushButton("↺ Reset", &dialog);
    btnStart->setMinimumHeight(35);
    btnReset->setMinimumHeight(35);

    layoutPrzyciskiStopera->addWidget(btnStart);
    layoutPrzyciskiStopera->addWidget(btnReset);

    layoutCzas->addWidget(lblStoper);
    layoutCzas->addLayout(layoutPrzyciskiStopera);

    // Spinboxy ręcznej korekty zsynchronizowane ze stoperem w obie strony.
    auto* layoutManual = new QHBoxLayout();
    auto* spinH = new QSpinBox(&dialog); spinH->setRange(0, 999); spinH->setSuffix(" h");
    auto* spinM = new QSpinBox(&dialog); spinM->setRange(0, 59); spinM->setSuffix(" m");
    auto* spinS = new QSpinBox(&dialog); spinS->setRange(0, 59); spinS->setSuffix(" s");

    layoutManual->addWidget(new QLabel("Korekta ręczna:", &dialog));
    layoutManual->addStretch();
    layoutManual->addWidget(spinH);
    layoutManual->addWidget(spinM);
    layoutManual->addWidget(spinS);
    layoutCzas->addLayout(layoutManual);

    mainLayout->addWidget(groupCzas);

    // --- Sekcja: Notatka ---
    auto* formNotatka = new QFormLayout();
    auto* txtNotatka = new QTextEdit(&dialog);
    txtNotatka->setPlainText(notatka);
    txtNotatka->setPlaceholderText("Coś ciekawego się wydarzyło? (Opcjonalnie)");
    txtNotatka->setMinimumHeight(80);
    formNotatka->addRow("Notatka:", txtNotatka);
    mainLayout->addLayout(formNotatka);

    // --- Przyciski Zapisz/Anuluj ---
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    mainLayout->addWidget(buttons);

    // ==========================================
    // LOGIKA STOPERA I SYNCHRONIZACJI
    // ==========================================
    int totalSeconds = sekundy;

    auto updateDisplay = [&]() {
        int h = totalSeconds / 3600;
        int m = (totalSeconds % 3600) / 60;
        int s = totalSeconds % 60;

        // Blokujemy sygnały w SpinBoxach żeby nie wywołać nieskończonej pętli aktualizacji
        spinH->blockSignals(true); spinM->blockSignals(true); spinS->blockSignals(true);
        spinH->setValue(h); spinM->setValue(m); spinS->setValue(s);
        spinH->blockSignals(false); spinM->blockSignals(false); spinS->blockSignals(false);

        lblStoper->setText(QString("%1:%2:%3")
                               .arg(h, 2, 10, QChar('0'))
                               .arg(m, 2, 10, QChar('0'))
                               .arg(s, 2, 10, QChar('0')));
    };
    updateDisplay();

    QTimer* timer = new QTimer(&dialog);
    connect(timer, &QTimer::timeout, [&]() {
        totalSeconds++;
        updateDisplay();
    });

    connect(btnStart, &QPushButton::clicked, [&]() {
        if (timer->isActive()) {
            timer->stop();
            btnStart->setText("▶ Start");
            btnStart->setStyleSheet("");
        } else {
            timer->start(1000);
            btnStart->setText("⏸ Pauza");
            btnStart->setStyleSheet("background-color: #c0392b; color: white;");
        }
    });

    connect(btnReset, &QPushButton::clicked, [&]() {
        totalSeconds = 0;
        updateDisplay();
        if (!timer->isActive()) {
            btnStart->setText("▶ Start");
        }
    });

    auto manualChanged = [&]() {
        totalSeconds = spinH->value() * 3600 + spinM->value() * 60 + spinS->value();
        updateDisplay();
    };
    connect(spinH, QOverload<int>::of(&QSpinBox::valueChanged), manualChanged);
    connect(spinM, QOverload<int>::of(&QSpinBox::valueChanged), manualChanged);
    connect(spinS, QOverload<int>::of(&QSpinBox::valueChanged), manualChanged);

    // ==========================================
    // ZATWIERDZANIE
    // ==========================================
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    // Przypisanie wyników do zmiennych wyjściowych
    przyrost = spinPrzyrost->value();
    sekundy = totalSeconds;
    notatka = txtNotatka->toPlainText().trimmed();

    if (!edycja && przyrost == 0 && sekundy == 0 && notatka.isEmpty()) {
        QMessageBox::warning(this, "Puste dane", "Sesja nie może być całkowicie pusta.\nMusisz spędzić chociaż sekundę, zrobić postęp lub dodać notatkę.");
        return false;
    }
    return true;
}

bool SzczegolyWidget::pokazDialogPodejscia(QString& status, int& aktualna, int& docelowa, int& ocena, QString& recenzja, const QString& tytul) {
    QDialog dialog(this);
    dialog.setWindowTitle(tytul);
    auto* layout = new QVBoxLayout(&dialog);
    auto* form = new QFormLayout();

    auto* comboStatus = new QComboBox(&dialog);
    comboStatus->addItems(appController.pobierzDostepneStatusy());
    comboStatus->setCurrentText(status);

    auto* spinAktualna = new QSpinBox(&dialog);
    spinAktualna->setRange(0, 9999);
    spinAktualna->setValue(aktualna);

    auto* spinDocelowa = new QSpinBox(&dialog);
    spinDocelowa->setRange(0, 9999);
    spinDocelowa->setValue(docelowa);

    auto* spinOcena = new QSpinBox(&dialog);
    spinOcena->setRange(0, 10);
    spinOcena->setValue(ocena);

    auto* txtRecenzja = new QTextEdit(&dialog);
    txtRecenzja->setPlainText(recenzja);
    txtRecenzja->setMinimumHeight(90);

    form->addRow("Status:", comboStatus);
    form->addRow("Postęp aktualny:", spinAktualna);
    form->addRow("Postęp docelowy:", spinDocelowa);
    form->addRow("Ocena (0-10):", spinOcena);
    form->addRow("Recenzja:", txtRecenzja);
    layout->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    status = comboStatus->currentText();
    aktualna = spinAktualna->value();
    docelowa = spinDocelowa->value();
    ocena = spinOcena->value();
    recenzja = txtRecenzja->toPlainText().trimmed();
    return true;
}

void SzczegolyWidget::onBtnEdytujZaznaczoneClicked() {
    QTreeWidgetItem* item = ui->treeHistoria->currentItem();
    if (!item) return;

    const auto typ = static_cast<TypHistoriiElementu>(item->data(0, ROLE_TYP).toInt());

    if (typ == TypHistoriiElementu::Podejscie) {
        const int idPodejscia = item->data(0, ROLE_ID).toInt();
        const auto historia = appController.pobierzHistorie(aktualneIdMedium);

        for (const auto& p : historia) {
            if (p.id == idPodejscia) {
                // Szybki dialog edycji
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
                    appController.aktualizujPodejscie(idPodejscia, p.status, p.aktualna, p.docelowa, s.value(), t.toPlainText().trimmed());
                    ustawMedium(aktualneIdMedium);
                    emit daneZaktualizowane();
                }
                return;
            }
        }
    } else if (typ == TypHistoriiElementu::Sesja) {
        const int idSesji = item->data(0, ROLE_ID).toInt();
        const auto historia = appController.pobierzHistorie(aktualneIdMedium);

        for (const auto& p : historia) {
            for (const auto& s : p.sesje) {
                if (s.id != idSesji) continue;

                int przyrost = s.przyrost;
                int sekundy = s.sekundy;
                QString notatka = s.notatka;

                // Otwieramy nasz mądry formularz ze stoperem (w trybie edycji)
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

void SzczegolyWidget::onBtnUsunZaznaczoneClicked() {
    QTreeWidgetItem* item = ui->treeHistoria->currentItem();
    if (!item) {
        return;
    }

    const auto typ = static_cast<TypHistoriiElementu>(item->data(0, ROLE_TYP).toInt());
    QString etykieta = (typ == TypHistoriiElementu::Podejscie) ? "podejście" : "sesję";
    QString potwierdzenie = QInputDialog::getText(
        this,
        "Potwierdzenie usunięcia",
        QString("Aby usunąć %1, wpisz dokładnie: USUŃ").arg(etykieta)
    );
    if (potwierdzenie != "USUŃ") {
        QMessageBox::information(this, "Anulowano", "Usuwanie anulowane.");
        return;
    }

    bool ok = false;
    if (typ == TypHistoriiElementu::Podejscie) {
        ok = appController.usunPodejscie(item->data(0, ROLE_ID).toInt());
    } else if (typ == TypHistoriiElementu::Sesja) {
        ok = appController.usunSesje(item->data(0, ROLE_ID).toInt());
    }

    if (!ok) {
        QMessageBox::warning(this, "Błąd", "Nie udało się usunąć zaznaczonego elementu.");
        return;
    }

    ustawMedium(aktualneIdMedium);
    emit daneZaktualizowane();
}

void SzczegolyWidget::odswiezPrzyciskUlubione(bool czyUlubione) {
    ui->btnUlubione->setText(czyUlubione ? "❤" : "♡");
    ui->btnUlubione->setToolTip(czyUlubione ? "Usuń z ulubionych" : "Dodaj do ulubionych");
    ui->btnUlubione->setStyleSheet(
        czyUlubione
            ? "QPushButton { color: #e74c3c; font-size: 20px; font-weight: 700; border: none; background: transparent; }"
            : "QPushButton { color: #8a8f98; font-size: 20px; font-weight: 700; border: none; background: transparent; }"
    );
}

void SzczegolyWidget::onBtnUlubioneClicked() {
    if (aktualneIdMedium <= 0) {
        return;
    }

    bool obecnyStan = (ui->btnUlubione->text() == "❤");
    bool nowyStan = !obecnyStan;
    if (!appController.ustawUlubione(aktualneIdMedium, nowyStan)) {
        QMessageBox::warning(this, "Błąd", "Nie udało się zaktualizować statusu ulubionych.");
        return;
    }

    odswiezPrzyciskUlubione(nowyStan);
}

void SzczegolyWidget::onBtnSzybkaSesjaClicked() {
    // Pobieramy ID aktualnego (ostatniego) podejścia z historii
    auto historia = appController.pobierzHistorie(aktualneIdMedium);
    if (historia.isEmpty()) return;

    int idPodejscia = historia.first().id;
    int przyrost = 0; int sekundy = 0; QString notatka;

    if (pokazDialogSesji(przyrost, sekundy, notatka, "Szybkie dodawanie aktywności", false)) {
        if (appController.dodajSesje(idPodejscia, przyrost, sekundy, notatka)) {
            ustawMedium(aktualneIdMedium);
            emit daneZaktualizowane();
        }
    }
}


void SzczegolyWidget::onBtnNowePodejscieClicked() {
    if (aktualneIdMedium == -1) return;

    bool ok;
    int nowyCel = QInputDialog::getInt(
        this, "Nowe podejście",
        "Świetnie, że dajesz temu tytułowi nową szansę!\nPodaj cel dla nowego podejścia:",
        ui->spinDetaleDocelowy->value(), 1, 9999, 1, &ok
        );

    if (ok) {
        if (appController.dodajPodejscie(aktualneIdMedium, "W trakcie", nowyCel)) {
            ustawMedium(aktualneIdMedium);
            emit daneZaktualizowane();
        }
    }
}

void SzczegolyWidget::onBtnCofnijZakonczenieClicked() {
    auto historia = appController.pobierzHistorie(aktualneIdMedium);
    if (historia.isEmpty()) return;

    int idPodejscia = historia.first().id;

    if (QMessageBox::question(this, "Cofnij zakończenie",
                              "Czy na pewno chcesz cofnąć zakończenie i przywrócić to podejście do statusu 'W trakcie'?",
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        // Przywracamy status, zachowując aktualne statystyki
        if (appController.aktualizujPodejscie(idPodejscia, "W trakcie", historia.first().aktualna, historia.first().docelowa, historia.first().ocena, historia.first().recenzja)) {
            ustawMedium(aktualneIdMedium);
            emit daneZaktualizowane();
        }
    }
}

void SzczegolyWidget::onBtnZakonczPodejscieClicked() {
    int akt = ui->spinDetaleAktualny->value();
    int doc = ui->spinDetaleDocelowy->value();
    bool osiagnietoCel = (akt >= doc && doc > 0);

    auto historia = appController.pobierzHistorie(aktualneIdMedium);
    QString istniejącaRecenzja = "";
    if (!historia.isEmpty()) istniejącaRecenzja = historia.first().recenzja;

    // Budowa okna dialogowego
    QDialog dialog(this);
    dialog.setWindowTitle("Podsumowanie podejścia");
    dialog.resize(450, 350);
    QVBoxLayout layout(&dialog);

    QLabel lblKomunikat;
    if (osiagnietoCel) {
        lblKomunikat.setText("<span style='font-size: 14px; color: #27ae60;'><b>Gratulacje!</b> Osiągnąłeś założony cel.</span><br>Podsumuj swoje doświadczenie:");
    } else {
        lblKomunikat.setText("<span style='font-size: 14px; color: #e74c3c;'><b>Nie osiągnąłeś celu.</b></span><br>Oznacz podejście jako porzucone lub wymuś ukończenie:");
    }
    lblKomunikat.setTextFormat(Qt::RichText);
    layout.addWidget(&lblKomunikat);

    QFormLayout form;

    QComboBox comboStatus;
    comboStatus.addItems({"Ukończone", "Porzucone"});
    comboStatus.setCurrentText(osiagnietoCel ? "Ukończone" : "Porzucone");

    QSpinBox spinOcena;
    spinOcena.setRange(0, 10);
    spinOcena.setSpecialValueText("Brak oceny"); // Jeśli użytkownik zostawi 0, zapisze się w bazie jako NULL/0
    spinOcena.setValue(ui->lblDetaleOcena->value());

    QTextEdit txtRecenzja;
    txtRecenzja.setPlainText(istniejącaRecenzja);
    txtRecenzja.setPlaceholderText("Dopisz coś lub zmień podsumowanie...");

    form.addRow("Status końcowy:", &comboStatus);
    form.addRow("Ostateczna ocena (1-10):", &spinOcena);
    form.addRow("Recenzja:", &txtRecenzja);
    layout.addLayout(&form);

    QDialogButtonBox buttons(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    buttons.button(QDialogButtonBox::Save)->setText("Zapieczętuj podejście");
    layout.addWidget(&buttons);

    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        auto historia = appController.pobierzHistorie(aktualneIdMedium);
        if (historia.isEmpty()) return;

        int idPodejscia = historia.first().id;
        QString finalStatus = comboStatus.currentText();
        int finalOcena = spinOcena.value();
        QString finalRecenzja = txtRecenzja.toPlainText().trimmed();

        if (appController.aktualizujPodejscie(idPodejscia, finalStatus, akt, doc, finalOcena, finalRecenzja)) {
            ustawMedium(aktualneIdMedium);
            emit daneZaktualizowane();
        }
    }
}
