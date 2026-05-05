#include "szczegolywidget.h"
#include "ui_szczegolywidget.h"
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

    ui->treeHistoria->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

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
    connect(ui->btnZacznijOdNowa, &QPushButton::clicked, this, &SzczegolyWidget::onBtnZacznijOdNowaClicked);
    connect(ui->btnDodajPodejscie, &QPushButton::clicked, this, &SzczegolyWidget::onBtnDodajPodejscieClicked);
    connect(ui->btnDodajSesje, &QPushButton::clicked, this, &SzczegolyWidget::onBtnDodajSesjeClicked);
    connect(ui->btnEdytujZaznaczone, &QPushButton::clicked, this, &SzczegolyWidget::onBtnEdytujZaznaczoneClicked);
    connect(ui->btnUsunZaznaczone, &QPushButton::clicked, this, &SzczegolyWidget::onBtnUsunZaznaczoneClicked);
    connect(ui->btnUlubione, &QPushButton::clicked, this, &SzczegolyWidget::onBtnUlubioneClicked);

    aktualizujStanPrzyciskowHistorii();
}

SzczegolyWidget::~SzczegolyWidget()
{
    delete ui;
}

void SzczegolyWidget::ustawMedium(int idMedium) {
    aktualneIdMedium = idMedium;
    ui->tabWidget->setCurrentIndex(0);

    auto lista = appController.pobierzWszystkieMultimedia();
    for (const auto& medium : lista) {
        if (medium->getId() == idMedium) {
            ui->lblDetaleTytul->setText(medium->getTytul());

            auto platformy = appController.pobierzPlatformy();
            QString nazwaPlat = "Nieznana platforma";
            for (const auto& p : platformy) {
                if (p.first == medium->getIdPlatformy()) { nazwaPlat = p.second; break; }
            }
            ui->labNazwaPlatformy->setText(nazwaPlat);
            odswiezPrzyciskUlubione(medium->getCzyUlubione());

            if (medium->getStatus() == "Ukończone") {
                int nr = medium->getPostep().numer_podejscia;
                QString tekstUkonczenia = (nr > 1) ? QString("UKOŃCZONO (x%1)").arg(nr) : "UKOŃCZONO";
                ui->lblWielkiStatus->setText(tekstUkonczenia);
                ui->lblWielkiStatus->show();
                ui->btnZacznijOdNowa->show();
                ui->frameKontrolkiPostepu->hide();
                ui->progressBarDetale->hide();
                ui->comboDetaleStatus->hide();
                ui->label_2->hide();
            } else {
                ui->lblWielkiStatus->hide();
                ui->btnZacznijOdNowa->hide();
                ui->frameKontrolkiPostepu->show();
                ui->progressBarDetale->show();
                ui->comboDetaleStatus->show();
                ui->label_2->show();
                ui->comboDetaleStatus->setCurrentText(medium->getStatus());
            }

            int procent = static_cast<int>(medium->getPostep().getProcent());
            ui->progressBarDetale->setValue(procent);

            ui->lblDetaleOcena->setValue(medium->getOcena());
            ui->lblDetaleOcena->show();
            ui->label_4->show();

            QString dDodania = medium->getDataDodania().toString("dd.MM.yyyy");
            QString dEdycji = medium->getDataOstatniejAktywnosci().toString("dd.MM.yyyy HH:mm");

            ui->spinDetaleDocelowy->setValue(medium->getPostep().docelowa);
            ui->spinDetaleAktualny->setMaximum(medium->getPostep().docelowa);
            ui->spinDetaleAktualny->setValue(medium->getPostep().aktualna);
            ui->lblDetaleJednostka->setText(medium->getPostep().jednostka);

            auto historia = appController.pobierzHistorie(idMedium);
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
            ui->lblDetaleCzasCalkowity->setText(
                QString("Łączny czas: %1h %2m")
                    .arg(sumaSekundCalkowita / 3600)
                    .arg((sumaSekundCalkowita % 3600) / 60)
                );

            QDateTime teraz = QDateTime::currentDateTime();
            QString tekstLicznikow;

            QString tooltipDaty = QString("Dodano: %1\nOstatnia aktywność: %2")
                                      .arg(medium->getDataDodania().toString("dd.MM.yyyy"))
                                      .arg(medium->getDataOstatniejAktywnosci().toString("dd.MM.yyyy HH:mm"));

            if (medium->getStatus() == "Ukończone") {
                ui->lblLicznikiZaniedbania->hide();
            } else {
                ui->lblLicznikiZaniedbania->show();
                int dniOdDodania = medium->getDataDodania().daysTo(teraz);

                tekstLicznikow = QString("W bibliotece od: <b>%1 dni</b>").arg(dniOdDodania);

                if (medium->getStatus() == "W trakcie" && dataStartuPodejscia.isValid()) {
                    int dniOdStartu = dataStartuPodejscia.daysTo(teraz);
                    int dniBezAkcji = medium->getDataOstatniejAktywnosci().daysTo(teraz);

                    QString kolorBrakPostepu = (dniBezAkcji > 30) ? "#e74c3c" : palette().color(QPalette::WindowText).name();

                    tekstLicznikow += QString(" | Podejście aktywne od: <b>%1 dni</b> | Brak postępu od: <b style='color:%2;'>%3 dni</b>")
                                          .arg(dniOdStartu)
                                          .arg(kolorBrakPostepu)
                                          .arg(dniBezAkcji);

                    tooltipDaty += QString("\nStart podejścia: %1").arg(dataStartuPodejscia.toString("dd.MM.yyyy HH:mm"));

                } else if (medium->getStatus() == "Wstrzymane") {
                    int dniPrzestoju = medium->getDataOstatniejAktywnosci().daysTo(teraz);
                    tekstLicznikow += QString(" | Wstrzymane od: <b>%1 dni</b>").arg(dniPrzestoju);
                }
            }

            ui->lblLicznikiZaniedbania->setToolTip(tooltipDaty);
            ui->lblLicznikiZaniedbania->setText(tekstLicznikow);

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

void SzczegolyWidget::onBtnZacznijOdNowaClicked() {
    if (aktualneIdMedium == -1) return;

    if (QMessageBox::question(this, "Zacznij od nowa",
                              "Czy na pewno chcesz wyzerować postęp i zacząć od nowa?\nTa operacja doda +1 do licznika powtórek.",
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
        return;
    }

    if (appController.zacznijOdNowa(aktualneIdMedium)) { // ZMIANA
        ustawMedium(aktualneIdMedium);
        emit daneZaktualizowane();
        QMessageBox::information(this, "Sukces", "Licznik wyzerowany. Lecimy od nowa!");
    }
}

void SzczegolyWidget::odswiezHistorie(int idMedium) {
    ui->treeHistoria->clear();
    ui->txtSzczegolyWpisu->clear();

    auto historia = appController.pobierzHistorie(idMedium);

    for (const auto& p : historia) {
        // 1. Tworzymy węzeł podejścia
        QTreeWidgetItem *pNode = new QTreeWidgetItem(ui->treeHistoria);
        pNode->setText(0, QString("Podejście #%1 (%2)").arg(p.numer).arg(p.status));
        pNode->setData(0, ROLE_TYP, static_cast<int>(TypHistoriiElementu::Podejscie));
        pNode->setData(0, ROLE_ID, p.id);

        // 2. Ustawiamy postęp (kolumna 2)
        pNode->setText(2, QString("%1/%2").arg(p.aktualna).arg(p.docelowa));

        // 3. Ustawiamy DATĘ ROZPOCZĘCIA (kolumna 1) - i NIE nadpisujemy jej później!
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

        // 4. Ustawiamy ZSUMOWANY CZAS dla podejścia (kolumna 3)
        int sumH = sumaSekund / 3600;
        int sumM = (sumaSekund % 3600) / 60;
        pNode->setText(3, QString("%1h %2m").arg(sumH).arg(sumM));

        // 5. Zapisujemy recenzję do podglądu
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
        ui->btnDodajSesje->setEnabled(false);
        ui->btnEdytujZaznaczone->setEnabled(false);
        ui->btnUsunZaznaczone->setEnabled(false);
        ui->btnEdytujZaznaczone->setText("Edytuj");
        ui->btnUsunZaznaczone->setText("Usuń");
        return;
    }

    const auto typ = static_cast<TypHistoriiElementu>(item->data(0, ROLE_TYP).toInt());
    if (typ == TypHistoriiElementu::Podejscie) {
        ui->btnDodajSesje->setEnabled(true);
        ui->btnEdytujZaznaczone->setEnabled(true);
        ui->btnUsunZaznaczone->setEnabled(true);
        ui->btnEdytujZaznaczone->setText("Edytuj podejście");
        ui->btnUsunZaznaczone->setText("Usuń podejście");
    } else if (typ == TypHistoriiElementu::Sesja) {
        ui->btnDodajSesje->setEnabled(true);
        ui->btnEdytujZaznaczone->setEnabled(true);
        ui->btnUsunZaznaczone->setEnabled(true);
        ui->btnEdytujZaznaczone->setText("Edytuj sesję");
        ui->btnUsunZaznaczone->setText("Usuń sesję");
    } else {
        ui->btnDodajSesje->setEnabled(false);
        ui->btnEdytujZaznaczone->setEnabled(false);
        ui->btnUsunZaznaczone->setEnabled(false);
    }
}

bool SzczegolyWidget::pokazDialogSesji(int& przyrost, int& sekundy, QString& notatka, const QString& tytul, bool edycja) {
    QDialog dialog(this);
    dialog.setWindowTitle(tytul);
    auto* layout = new QVBoxLayout(&dialog);
    auto* form = new QFormLayout();

    auto* spinPrzyrost = new QSpinBox(&dialog);
    spinPrzyrost->setRange(-9999, 9999);
    spinPrzyrost->setValue(przyrost);

    auto* spinMinuty = new QSpinBox(&dialog);
    spinMinuty->setRange(0, 24 * 60);
    spinMinuty->setValue(sekundy / 60);

    auto* txtNotatka = new QTextEdit(&dialog);
    txtNotatka->setPlainText(notatka);
    txtNotatka->setPlaceholderText("Opcjonalna notatka do sesji...");
    txtNotatka->setMinimumHeight(90);

    form->addRow("Przyrost jednostek:", spinPrzyrost);
    form->addRow("Czas trwania [min]:", spinMinuty);
    form->addRow("Notatka:", txtNotatka);
    layout->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    przyrost = spinPrzyrost->value();
    sekundy = spinMinuty->value() * 60;
    notatka = txtNotatka->toPlainText().trimmed();
    if (!edycja && przyrost == 0 && sekundy == 0 && notatka.isEmpty()) {
        QMessageBox::warning(this, "Puste dane", "Sesja nie może być całkowicie pusta.");
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

void SzczegolyWidget::onBtnDodajPodejscieClicked() {
    if (aktualneIdMedium <= 0) {
        return;
    }

    QString status = "Planowane";
    int aktualna = 0;
    int docelowa = qMax(1, ui->spinDetaleDocelowy->value());
    int ocena = 0;
    QString recenzja;

    if (!pokazDialogPodejscia(status, aktualna, docelowa, ocena, recenzja, "Dodaj nowe podejście")) {
        return;
    }

    if (!appController.dodajPodejscie(aktualneIdMedium, status, docelowa)) {
        QMessageBox::warning(this, "Błąd", "Nie udało się dodać podejścia.");
        return;
    }

    auto historia = appController.pobierzHistorie(aktualneIdMedium);
    if (!historia.isEmpty()) {
        const auto& najnowsze = historia.first();
        appController.aktualizujPodejscie(najnowsze.id, status, aktualna, docelowa, ocena, recenzja);
    }

    ustawMedium(aktualneIdMedium);
    emit daneZaktualizowane();
}

void SzczegolyWidget::onBtnDodajSesjeClicked() {
    QTreeWidgetItem* item = ui->treeHistoria->currentItem();
    if (!item) {
        return;
    }

    int idPodejscia = -1;
    const auto typ = static_cast<TypHistoriiElementu>(item->data(0, ROLE_TYP).toInt());
    if (typ == TypHistoriiElementu::Podejscie) {
        idPodejscia = item->data(0, ROLE_ID).toInt();
    } else if (typ == TypHistoriiElementu::Sesja) {
        idPodejscia = item->data(0, ROLE_ID_PODEJSCIA).toInt();
    }
    if (idPodejscia <= 0) {
        return;
    }

    int przyrost = 0;
    int sekundy = 0;
    QString notatka;
    if (!pokazDialogSesji(przyrost, sekundy, notatka, "Dodaj sesję", false)) {
        return;
    }

    if (!appController.dodajSesje(idPodejscia, przyrost, sekundy, notatka)) {
        QMessageBox::warning(this, "Błąd", "Nie udało się dodać sesji.");
        return;
    }

    ustawMedium(aktualneIdMedium);
    emit daneZaktualizowane();
}

void SzczegolyWidget::onBtnEdytujZaznaczoneClicked() {
    QTreeWidgetItem* item = ui->treeHistoria->currentItem();
    if (!item) {
        return;
    }

    const auto typ = static_cast<TypHistoriiElementu>(item->data(0, ROLE_TYP).toInt());
    if (typ == TypHistoriiElementu::Podejscie) {
        const int idPodejscia = item->data(0, ROLE_ID).toInt();
        auto historia = appController.pobierzHistorie(aktualneIdMedium);
        for (const auto& p : historia) {
            if (p.id != idPodejscia) {
                continue;
            }
            QString status = p.status;
            int aktualna = p.aktualna;
            int docelowa = p.docelowa;
            int ocena = p.ocena;
            QString recenzja = p.recenzja;
            if (!pokazDialogPodejscia(status, aktualna, docelowa, ocena, recenzja, "Edytuj podejście")) {
                return;
            }
            if (!appController.aktualizujPodejscie(idPodejscia, status, aktualna, docelowa, ocena, recenzja)) {
                QMessageBox::warning(this, "Błąd", "Nie udało się zaktualizować podejścia.");
                return;
            }
            ustawMedium(aktualneIdMedium);
            emit daneZaktualizowane();
            return;
        }
    } else if (typ == TypHistoriiElementu::Sesja) {
        const int idSesji = item->data(0, ROLE_ID).toInt();
        auto historia = appController.pobierzHistorie(aktualneIdMedium);
        for (const auto& p : historia) {
            for (const auto& s : p.sesje) {
                if (s.id != idSesji) {
                    continue;
                }
                int przyrost = s.przyrost;
                int sekundy = s.sekundy;
                QString notatka = s.notatka;
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
