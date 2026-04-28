#include "szczegolywidget.h"
#include "ui_szczegolywidget.h"
#include <QMessageBox>

SzczegolyWidget::SzczegolyWidget(AppController& controller, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SzczegolyWidget),
    appController(controller)
{
    ui->setupUi(this);

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

    connect(ui->treeHistoria, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int column) {
        ui->txtSzczegolyWpisu->setHtml(item->data(0, Qt::UserRole).toString());
    });

    connect(ui->btnDetaleZapisz, &QPushButton::clicked, this, &SzczegolyWidget::onBtnZapiszClicked);
    connect(ui->btnZacznijOdNowa, &QPushButton::clicked, this, &SzczegolyWidget::onBtnZacznijOdNowaClicked);
}

SzczegolyWidget::~SzczegolyWidget()
{
    delete ui;
}

void SzczegolyWidget::ustawMedium(int idMedium) {
    aktualneIdMedium = idMedium;

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
    odswiezHistorie(idMedium);
}

void SzczegolyWidget::onBtnZapiszClicked() {
    if (aktualneIdMedium == -1) return;

    QString nowyStatus = ui->comboDetaleStatus->currentText();
    int nowaAktualna = ui->spinDetaleAktualny->value();
    int nowaDocelowa = ui->spinDetaleDocelowy->value();
    int ocena = ui->lblDetaleOcena->value();

    if (appController.czyOsiagnietoCel(nowaAktualna, nowaDocelowa)) {
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
        // Tworzymy węzeł podejścia
        QTreeWidgetItem *pNode = new QTreeWidgetItem(ui->treeHistoria);
        pNode->setText(0, QString("Podejście #%1 (%2)").arg(p.numer).arg(p.status));
        pNode->setText(2, QString("%1/%2").arg(p.aktualna).arg(p.docelowa));
        pNode->setBackground(0, QColor("#f0f0f0"));

        // Zapisujemy recenzję w UserRole, żeby wyświetlić ją po kliknięciu
        QString infoPodejscie = QString("<b>Status:</b> %1<br><b>Ocena:</b> %2/10<br><br><b>Recenzja:</b><br>%3")
                                    .arg(p.status).arg(p.ocena > 0 ? QString::number(p.ocena) : "brak")
                                    .arg(p.recenzja.isEmpty() ? "Brak recenzji." : p.recenzja);
        pNode->setData(0, Qt::UserRole, infoPodejscie);

        for (const auto& s : p.sesje) {
            QTreeWidgetItem *sNode = new QTreeWidgetItem(pNode);
            sNode->setText(0, "Sesja");
            sNode->setText(1, s.data.toString("dd.MM.yyyy HH:mm"));
            sNode->setText(2, QString("+%1").arg(s.przyrost));

            // Konwersja sekund na format H:M
            int h = s.sekundy / 3600;
            int m = (s.sekundy % 3600) / 60;
            sNode->setText(3, QString("%1h %2m").arg(h).arg(m));
            sNode->setText(4, s.notatka);

            // Zapisujemy pełną notatkę sesji
            QString infoSesja = QString("<b>Data:</b> %1<br><b>Przyrost:</b> %2<br><b>Czas trwania:</b> %3h %4m<br><br><b>Notatka:</b><br>%5")
                                    .arg(s.data.toString("dd.MM.yyyy HH:mm")).arg(s.przyrost).arg(h).arg(m)
                                    .arg(s.notatka.isEmpty() ? "Brak notatki dla tej sesji." : s.notatka);
            sNode->setData(0, Qt::UserRole, infoSesja);
        }
    }
    ui->treeHistoria->expandAll();
}
