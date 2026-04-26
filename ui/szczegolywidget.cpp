#include "szczegolywidget.h"
#include "ui_szczegolywidget.h"
#include <QMessageBox>

SzczegolyWidget::SzczegolyWidget(DatabaseManager& db, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SzczegolyWidget),
    dbManager(db)
{
    ui->setupUi(this);

    // Dodajemy opcje, których zapomniałeś przenieść do UI
    if (ui->comboDetaleStatus->count() == 0) {
        ui->comboDetaleStatus->addItem("Planowane");
        ui->comboDetaleStatus->addItem("W trakcie");
        ui->comboDetaleStatus->addItem("Porzucone");
    }

    // Twoje lambdy od przeliczania paska postępu - wreszcie na swoim miejscu!
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

    // Podpięcie przycisków
    connect(ui->btnDetaleZapisz, &QPushButton::clicked, this, &SzczegolyWidget::onBtnZapiszClicked);
    connect(ui->btnZacznijOdNowa, &QPushButton::clicked, this, &SzczegolyWidget::onBtnZacznijOdNowaClicked);
}

SzczegolyWidget::~SzczegolyWidget()
{
    delete ui;
}

void SzczegolyWidget::ustawMedium(int idMedium) {
    aktualneIdMedium = idMedium;

    // Pobieramy świeże dane (Nadal pobierasz całą listę, co jest zbrodnią optymalizacyjną, ale zadziała)
    auto lista = dbManager.getAllMultimedia();
    for (const auto& medium : lista) {
        if (medium->getId() == idMedium) {
            ui->lblDetaleTytul->setText(medium->getTytul());

            auto platformy = dbManager.pobierzPlatformy();
            QString nazwaPlat = "Nieznana platforma";
            for (const auto& p : platformy) {
                if (p.first == medium->getIdPlatformy()) { nazwaPlat = p.second; break; }
            }
            ui->labNazwaPlatformy->setText(nazwaPlat);

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
            ui->lblDetaleDaty->setText(QString("Dodano: %1 | Ostatnia aktywność: %2").arg(dDodania).arg(dEdycji));

            ui->spinDetaleDocelowy->setValue(medium->getPostep().docelowa);
            ui->spinDetaleAktualny->setMaximum(medium->getPostep().docelowa);
            ui->spinDetaleAktualny->setValue(medium->getPostep().aktualna);
            ui->lblDetaleJednostka->setText(medium->getPostep().jednostka);

            break;
        }
    }
}

void SzczegolyWidget::onBtnZapiszClicked() {
    if (aktualneIdMedium == -1) return;

    QString nowyStatus = ui->comboDetaleStatus->currentText();
    int nowaAktualna = ui->spinDetaleAktualny->value();
    int nowaDocelowa = ui->spinDetaleDocelowy->value();
    int ocena = ui->lblDetaleOcena->value();

    if (nowaAktualna >= nowaDocelowa && nowaDocelowa > 0) {
        auto odpowiedz = QMessageBox::question(this, "Automatyczne ukończenie",
                                               "Wartość aktualna zrównała się z docelową. Po zapisaniu pozycja automatycznie zmieni status na 'Ukończone'.\nKontynuować?",
                                               QMessageBox::Yes | QMessageBox::No);
        if (odpowiedz == QMessageBox::No) return;
        nowyStatus = "Ukończone";
    }

    if (dbManager.aktualizujPostep(aktualneIdMedium, nowyStatus, nowaAktualna, nowaDocelowa, ocena)) {
        ustawMedium(aktualneIdMedium); // Odśwież widok lokalnie
        emit daneZaktualizowane();     // Krzyczymy do MainWindow
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

    if (dbManager.zacznijOdNowa(aktualneIdMedium)) {
        ustawMedium(aktualneIdMedium); // Odśwież widok lokalnie
        emit daneZaktualizowane();     // Krzyczymy do MainWindow
        QMessageBox::information(this, "Sukces", "Licznik wyzerowany. Lecimy od nowa!");
    }
}