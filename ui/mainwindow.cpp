#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "kategoriedialog.h"
#include "platformydialog.h"
#include "statisticswidget.h"
#include "multimediaformwidget.h"

#include <qlineedit.h>
#include <utility>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QLocale>
#include <QInputDialog>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QChartView>
#include <QPieSeries>
#include <QChart>
#include <QRandomGenerator>
#include <QStackedBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QToolTip>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Inicjalizacja kontrolek
    ui->comboDetaleStatus->addItem("Planowane");
    ui->comboDetaleStatus->addItem("W trakcie");
    ui->comboDetaleStatus->addItem("Porzucone");
    ui->comboGrupowanie->addItems({"Kategoria", "Status", "Platforma", "Data dodania"});

    ui->kategorie->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->kategorie->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // --- PROSTE LAMBDY (Zostawiamy, bo to tylko 1-2 linijki logiki UI) ---
    connect(ui->comboGrupowanie, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        zaladujDaneDoDrzewa();
    });

    connect(ui->btnDodajMedium, &QPushButton::clicked, this, [this]() { przygotujFormularz(-1, 0); });


    connect(ui->btnPowrot, &QPushButton::clicked, this, [this]() {
        ui->daneSzczegolowe->setCurrentWidget(ui->page);
        odswiezStatystykiGlowne();
    });

    // Paski postępu (te lambdy są okej, bo bezpośrednio modyfikują UI obok siebie)
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



    // --- STANDARDOWE POŁĄCZENIA SLOTÓW ---
    connect(ui->kategorie, &QTreeWidget::itemClicked, this, &MainWindow::onWybieranieElementuDrzewa);
    connect(ui->wyszukiwarka, &QLineEdit::textChanged, this, &MainWindow::onWyszukiwanie);
    connect(ui->kategorie, &QTreeWidget::customContextMenuRequested, this, &MainWindow::pokazMenuDrzewa);

    connect(ui->btnDetaleZapisz, &QPushButton::clicked, this, &MainWindow::onBtnDetaleZapiszClicked);
    connect(ui->btnZacznijOdNowa, &QPushButton::clicked, this, &MainWindow::onBtnZacznijOdNowaClicked);

    connect(ui->btnLosuj, &QPushButton::clicked, this, &MainWindow::onBtnLosujClicked);

    // --- AKCJE Z MENU GŁÓWNEGO ---
    connect(ui->actionKategorie, &QAction::triggered, this, [this]() {
        KategorieDialog dialog(dbManager, this);
        dialog.exec();
        listaMultimediow = dbManager.getAllMultimedia();
        zaladujDaneDoDrzewa();
        uzupelnijComboBoxy();
    });

    connect(ui->actionPlatformy, &QAction::triggered, this, [this]() {
        PlatformyDialog dialog(dbManager, this);
        dialog.exec();
        listaMultimediow = dbManager.getAllMultimedia();
        zaladujDaneDoDrzewa();
        uzupelnijComboBoxy();
    });

    connect(ui->actionStronaGlowna, &QAction::triggered, this, [this]() {
            ui->panelKategorii->show();
            ui->daneSzczegolowe->setCurrentWidget(ui->page);
            ui->comboWidokStatystyk->hide();
    });

    connect(ui->actionStatystyki, &QAction::triggered, this, [this]() {
        ui->panelKategorii->hide();
        // Przełączamy stos na nasz nowy widget!
        ui->daneSzczegolowe->setCurrentWidget(statsWidget);
        ui->comboWidokStatystyk->show();
        statsWidget->odswiezWykresAktywnosci();
    });


    // Ukrywamy na starcie, dopóki nie wejdziemy w statystyki
    ui->comboWidokStatystyk->hide();
    // --- START APLIKACJI ---
    if (dbManager.openConnection()) {
        zaladujDaneDoDrzewa();
        odswiezStatystykiGlowne();
        uzupelnijComboBoxy();
        ui->daneSzczegolowe->setCurrentIndex(0);
    } else {
        qDebug() << "Błąd połączenia z bazą danych.";
    }


    statsWidget = new StatisticsWidget(dbManager, this);
    ui->daneSzczegolowe->addWidget(statsWidget);

    formularzWidget = new MultimediaFormWidget(dbManager, this);
    ui->daneSzczegolowe->addWidget(formularzWidget);

    // Kiedy formularz krzyknie, że zapisał dane:
    connect(formularzWidget, &MultimediaFormWidget::daneZapisane, this, [this]() {
        listaMultimediow = dbManager.getAllMultimedia();
        zaladujDaneDoDrzewa();
        odswiezStatystykiGlowne();
        // Wracamy do widoku domyślnego
        ui->daneSzczegolowe->setCurrentWidget(ui->page);
    });

    // Kiedy formularz krzyknie, że go anulowano:
    connect(formularzWidget, &MultimediaFormWidget::formularzAnulowany, this, [this]() {
        ui->daneSzczegolowe->setCurrentWidget(ui->page);
    });

    connect(ui->btnDodajMedium, &QPushButton::clicked, this, [this]() {
        formularzWidget->przygotujFormularz(-1, 0, 0);
        ui->daneSzczegolowe->setCurrentWidget(formularzWidget); // Pokaż formularz!
    });
}

MainWindow::~MainWindow()
{
    listaMultimediow.clear();
    delete ui;
}

// ==========================================================
// METODY LOGIKI BIZNESOWEJ I UI
// ==========================================================


void MainWindow::onBtnLosujClicked() {
    QList<int> doWylosowania;
    for (const auto& m : std::as_const(listaMultimediow)) {
        if (m->getStatus() == "Planowane") doWylosowania.append(m->getId());
    }
    if (doWylosowania.isEmpty()) {
        QMessageBox::information(this, "Pusto!", "Nie masz żadnych 'Planowanych' pozycji.");
        return;
    }
    int wylosowaneId = doWylosowania.at(QRandomGenerator::global()->bounded(doWylosowania.size()));
    pokazSzczegolyMedium(wylosowaneId);
    ui->statusbar->showMessage("Oto twoje wylosowane przeznaczenie!", 4000);
}

void MainWindow::onBtnDetaleZapiszClicked() {
    auto wybrane = ui->kategorie->selectedItems();
    if (wybrane.isEmpty()) return;

    int id = wybrane.first()->data(0, Qt::UserRole).toInt();
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

    if (dbManager.aktualizujPostep(id, nowyStatus, nowaAktualna, nowaDocelowa, ocena)) {
        for(auto& medium : listaMultimediow) {
            if(medium->getId() == id) {
                medium->setStatus(nowyStatus);
                medium->setOcena(ocena);
                Postep p = medium->getPostep();
                p.aktualna = nowaAktualna;
                p.docelowa = nowaDocelowa;
                medium->setPostep(p);
                break;
            }
        }
        pokazSzczegolyMedium(id);
        odswiezStatystykiGlowne();
        ui->statusbar->showMessage("Zapisano zmiany w bazie!", 3000);
    }
}

void MainWindow::onBtnZacznijOdNowaClicked() {
    auto wybrane = ui->kategorie->selectedItems();
    if (wybrane.isEmpty()) return;

    int id = wybrane.first()->data(0, Qt::UserRole).toInt();

    if (QMessageBox::question(this, "Zacznij od nowa",
                              "Czy na pewno chcesz wyzerować postęp i zacząć od nowa?\nTa operacja doda +1 do licznika powtórek.",
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
        return;
    }

    if (dbManager.zacznijOdNowa(id)) {
        for(auto& medium : listaMultimediow) {
            if(medium->getId() == id) {
                medium->setStatus("W trakcie");
                Postep p = medium->getPostep();
                p.aktualna = 0;
                p.numer_podejscia++;
                medium->setPostep(p);
                break;
            }
        }
        pokazSzczegolyMedium(id);
        odswiezStatystykiGlowne();
        ui->statusbar->showMessage("Licznik wyzerowany. Lecimy od nowa!", 3000);
    }
}


void MainWindow::onWybieranieElementuDrzewa(QTreeWidgetItem *item, int column) {
    if (!item || item->parent() == nullptr) {
        if (item) item->setExpanded(!item->isExpanded());
        if (ui->daneSzczegolowe->currentWidget() != ui->page) {
            odswiezStatystykiGlowne();
            ui->daneSzczegolowe->setCurrentWidget(ui->page);
        }
        return;
    }
    int idWybranegoElementu = item->data(0, Qt::UserRole).toInt();
    pokazSzczegolyMedium(idWybranegoElementu);
}

void MainWindow::onWyszukiwanie(const QString &text) {
    if (ui->kategorie->topLevelItemCount() == 0) return;
    for (int i = 0; i < ui->kategorie->topLevelItemCount(); ++i) {
        QTreeWidgetItem *kategoria = ui->kategorie->topLevelItem(i);
        bool maPasujaceElementy = false;
        for (int j = 0; j < kategoria->childCount(); ++j) {
            QTreeWidgetItem *dziecko = kategoria->child(j);
            if (dziecko->text(0).contains(text, Qt::CaseInsensitive)) {
                dziecko->setHidden(false);
                maPasujaceElementy = true;
            } else {
                dziecko->setHidden(true);
            }
        }
        if (text.isEmpty()) {
            kategoria->setHidden(false);
        } else {
            kategoria->setHidden(!maPasujaceElementy);
            if (maPasujaceElementy) kategoria->setExpanded(true);
        }
    }
}

void MainWindow::zaladujDaneDoDrzewa() {
    ui->kategorie->clear();
    if (listaMultimediow.isEmpty()) listaMultimediow = dbManager.getAllMultimedia();

    int trybGrupowania = ui->comboGrupowanie->currentIndex();
    QMap<int, QString> slownikKategorii;
    QList<QPair<int, QString>> listaPlatform;
    if (trybGrupowania == 0) slownikKategorii = dbManager.getCategories();
    if (trybGrupowania == 2) listaPlatform = dbManager.pobierzPlatformy();

    QMap<QString, QTreeWidgetItem*> wezlyGlowne;

    if (trybGrupowania == 0) {
        QSqlQuery q("SELECT id, nazwa, jednostka FROM kategorie");
        while(q.next()) {
            QString nazwa = q.value(1).toString();
            QTreeWidgetItem *wezel = new QTreeWidgetItem(ui->kategorie);
            wezel->setText(0, nazwa);
            wezel->setData(0, Qt::UserRole, q.value(0).toInt());
            wezel->setData(0, Qt::UserRole + 1, q.value(2).toString());
            wezlyGlowne.insert(nazwa, wezel);
        }
    } else if (trybGrupowania == 2) {
        for (const auto& plat : listaPlatform) {
            QString nazwa = plat.second;
            QTreeWidgetItem *wezel = new QTreeWidgetItem(ui->kategorie);
            wezel->setText(0, nazwa);
            wezel->setData(0, Qt::UserRole, plat.first);
            wezlyGlowne.insert(nazwa, wezel);
        }
    }

    for (const auto& medium : std::as_const(listaMultimediow)) {
        QString nazwaGrupy;
        switch (trybGrupowania) {
        case 0: nazwaGrupy = slownikKategorii.value(medium->getIdKategorii(), "Brak kategorii"); break;
        case 1: nazwaGrupy = medium->getStatus(); break;
        case 2:
            nazwaGrupy = "Nieznana platforma";
            for (const auto& plat : listaPlatform) {
                if (plat.first == medium->getIdPlatformy()) { nazwaGrupy = plat.second; break; }
            }
            break;
        case 3:
            QLocale polski(QLocale::Polish, QLocale::Poland);
            QDate data = medium->getDataDodania().date();
            if (data.isValid()) {
                QString miesiac = polski.standaloneMonthName(data.month());
                nazwaGrupy = QString("%1 %2").arg(miesiac).arg(data.year()).toUpper();
            } else {
                nazwaGrupy = "BRAK DATY";
            }
            break;
        }

        if (!wezlyGlowne.contains(nazwaGrupy)) {
            QTreeWidgetItem *nowyWezel = new QTreeWidgetItem(ui->kategorie);
            nowyWezel->setText(0, nazwaGrupy);
            if (trybGrupowania == 0) {
                nowyWezel->setData(0, Qt::UserRole, medium->getIdKategorii());
                nowyWezel->setData(0, Qt::UserRole + 1, medium->getPostep().jednostka);
            } else if (trybGrupowania == 2) {
                nowyWezel->setData(0, Qt::UserRole, medium->getIdPlatformy());
            }
            wezlyGlowne.insert(nazwaGrupy, nowyWezel);
        }

        QTreeWidgetItem *item = new QTreeWidgetItem(wezlyGlowne[nazwaGrupy]);
        item->setText(0, medium->getTytul());
        item->setData(0, Qt::UserRole, medium->getId());
        item->setData(0, Qt::UserRole + 1, medium->getPostep().jednostka);
    }
    ui->kategorie->expandAll();
}

void MainWindow::odswiezStatystykiGlowne() {
    auto stats = dbManager.getGlobalStats();
    if (stats.value("Suma", 0) == 0) return;

    QPieSeries *series = new QPieSeries();
    series->append("Planowane", stats.value("Planowane", 0));
    series->append("W trakcie", stats.value("W trakcie", 0));
    series->append("Ukończone", stats.value("Ukończone", 0));
    series->append("Porzucone", stats.value("Porzucone", 0));

    series->slices().at(0)->setColor(QColor("#7f8c8d"));
    series->slices().at(1)->setColor(QColor("#2980b9"));
    series->slices().at(2)->setColor(QColor("#27ae60"));
    series->slices().at(3)->setColor(QColor("#c0392b"));

    for(auto slice : series->slices()) {
        if (slice->value() > 0) {
            slice->setLabelVisible(true);
            slice->setLabel(QString("%1: %2").arg(slice->label()).arg(slice->value()));
        }
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Status Biblioteki");
    chart->legend()->hide();
    chart->setAnimationOptions(QChart::SeriesAnimations);

    ui->wykresOwalny->setChart(chart);
    ui->wykresOwalny->setRenderHint(QPainter::Antialiasing);

    QLayoutItem *child;
    while ((child = ui->gridOstatnie->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    QList<int> ostatnieId = dbManager.pobierzOstatnioAktywne(6);
    int wiersz = 0, kolumna = 0;

    for (int id : ostatnieId) {
        for (const auto& m : std::as_const(listaMultimediow)) {
            if (m->getId() == id) {
                QPushButton *btnKafel = new QPushButton(m->getTytul(), this);
                btnKafel->setMinimumHeight(50);
                connect(btnKafel, &QPushButton::clicked, this, [this, id]() { pokazSzczegolyMedium(id); });
                ui->gridOstatnie->addWidget(btnKafel, wiersz, kolumna);
                kolumna++;
                if (kolumna > 1) { kolumna = 0; wiersz++; }
                break;
            }
        }
    }
}


void MainWindow::usunWybraneMedium(int id) {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Potwierdzenie",
                                  "Czy na pewno chcesz usunąć tę pozycję z biblioteki? \nTej operacji nie da się cofnąć.",
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        if (dbManager.usunMedium(id)) {
            ui->statusbar->showMessage("Usunięto pomyślnie", 3000);
            zaladujDaneDoDrzewa();
            ui->daneSzczegolowe->setCurrentIndex(0);
        }
    }
}


void MainWindow::pokazSzczegolyMedium(int idMedium) {
    for (const auto& medium : std::as_const(listaMultimediow)) {
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

            ui->daneSzczegolowe->setCurrentWidget(ui->page_2);
            break;
        }
    }
}

void MainWindow::pokazMenuDrzewa(const QPoint &pos) {
    QTreeWidgetItem *kliknietyElement = ui->kategorie->itemAt(pos);
    QMenu menu(this);

    // 0. SPRAWDZAMY, CZY ZAZNACZONO WIELE ELEMENTÓW
    QList<QTreeWidgetItem*> wybrane = ui->kategorie->selectedItems();

    if (wybrane.size() > 1) {
        // Filtrujemy zaznaczenie - upewniamy się, że to tylko pozycje (dzieci), a nie grupy
        QList<int> wybraneIds;
        for (auto *item : wybrane) {
            if (item->parent() != nullptr) { // Tylko elementy posiadające rodzica to gry/książki
                wybraneIds.append(item->data(0, Qt::UserRole).toInt());
            }
        }

        if (!wybraneIds.isEmpty() && ui->comboGrupowanie->currentIndex() == 0) { // Tylko w widoku kategorii
            menu.addAction(QString("Przenieś zaznaczone (%1) do innej kategorii...").arg(wybraneIds.size()), this, [this, wybraneIds]() {
                QDialog dialog(this);
                dialog.setWindowTitle("Zbiorcza zmiana kategorii");
                dialog.resize(300, 100);

                QFormLayout form(&dialog);
                QComboBox comboNowaKat(&dialog);

                // Ładujemy dostępne kategorie do ComboBoxa
                comboNowaKat.addItem("--- Wybierz nową kategorię ---", 0);
                auto kategorie = dbManager.pobierzKategorie();
                for (const auto& kat : std::as_const(kategorie)) {
                    if (kat.first != 0) comboNowaKat.addItem(kat.second, kat.first);
                }

                form.addRow("Wybierz docelową kategorię:", &comboNowaKat);
                QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
                form.addRow(&buttonBox);

                connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
                connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

                if (dialog.exec() == QDialog::Accepted) {
                    int noweIdKat = comboNowaKat.currentData().toInt();
                    if (noweIdKat == 0) return; // Zabezpieczenie przed wyborem placeholdera

                    if (dbManager.zmienKategorieWielu(wybraneIds, noweIdKat)) {
                        listaMultimediow = dbManager.getAllMultimedia(); // Pełne przeładowanie
                        zaladujDaneDoDrzewa();
                        ui->statusbar->showMessage(QString("Przeniesiono pomyślnie %1 elementów!").arg(wybraneIds.size()), 4000);
                    } else {
                        QMessageBox::critical(this, "Błąd", "Nie udało się przenieść elementów.");
                    }
                }
            });
        }
        // Skoro mamy wybraneIds (z filtrowania dla zmiany kategorii), używamy ich ponownie:
        if (!wybraneIds.isEmpty()) {
            menu.addSeparator();
            menu.addAction(QString("Usuń zaznaczone pozycje (%1)").arg(wybraneIds.size()), this, [this, wybraneIds]() {
                if (QMessageBox::question(this, "Masowe usuwanie",
                                          QString("Czy na pewno chcesz usunąć bezpowrotnie %1 pozycji z biblioteki?").arg(wybraneIds.size())) == QMessageBox::Yes) {

                    if (dbManager.usunWieleMultimediow(wybraneIds)) {
                        listaMultimediow = dbManager.getAllMultimedia();
                        zaladujDaneDoDrzewa();
                        odswiezStatystykiGlowne();
                        ui->daneSzczegolowe->setCurrentIndex(0); // Uciekamy na bezpieczny ekran
                        ui->statusbar->showMessage(QString("Skasowano %1 elementów.").arg(wybraneIds.size()), 4000);
                    } else {
                        QMessageBox::critical(this, "Błąd", "Baza danych zablokowała masowe usunięcie.");
                    }
                }
            });
        }

        // Zabezpieczenie: jeśli zaznaczono wiele elementów, nie pokazujemy reszty menu dla pojedynczego elementu
        if (!menu.isEmpty()) {
            menu.exec(ui->kategorie->mapToGlobal(pos));
        }
        return;
    }

    if (!kliknietyElement) {
        // 1. KLIKNIĘTO W PUSTE MIEJSCE (Pod listą)
        int trybGrupowania = ui->comboGrupowanie->currentIndex();

        if (trybGrupowania == 0) { // Tryb Kategorii
            menu.addAction("Dodaj nową kategorię...", this, [this]() {
                // Tworzymy własne, tymczasowe okienko dialogowe
                QDialog dialog(this);
                dialog.setWindowTitle("Nowa Kategoria");
                dialog.resize(300, 100);

                QFormLayout form(&dialog);
                QLineEdit editNazwa(&dialog);
                editNazwa.setPlaceholderText("np. Komiksy");

                QComboBox comboJednostka(&dialog);
                // TO JEST MAGIA UX: Combo staje się polem tekstowym, jeśli użytkownik tak zechce
                comboJednostka.setEditable(true);
                comboJednostka.addItems(dbManager.pobierzUnikalneJednostki());

                form.addRow("Nazwa kategorii:", &editNazwa);
                form.addRow("Jednostka:", &comboJednostka);

                // Dodajemy systemowe przyciski OK / Anuluj
                QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
                form.addRow(&buttonBox);

                // Łączymy przyciski z akcjami zamknięcia okna
                connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
                connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

                // Odpalamy okno. Jeśli użytkownik kliknie OK (Accepted):
                if (dialog.exec() == QDialog::Accepted) {
                    QString nazwa = editNazwa.text().trimmed();
                    QString jednostka = comboJednostka.currentText().trimmed(); // Pobierze to, co wybrano z listy LUB wpisano ręcznie!

                    if (!nazwa.isEmpty()) {
                        if (jednostka.isEmpty()) jednostka = "szt."; // Zabezpieczenie przed skrajną głupotą

                        int noweId = dbManager.dodajKategorie(nazwa, jednostka);
                        if (noweId > 0) {
                            uzupelnijComboBoxy();
                            int index = ui->comboNowaKategoria->findData(noweId);
                            if (index != -1) ui->comboNowaKategoria->setCurrentIndex(index);

                            // Jeśli wywołujesz to z poziomu drzewa, przyda się odświeżyć drzewo:
                            zaladujDaneDoDrzewa();

                            ui->statusbar->showMessage("Dodano kategorię!", 3000);
                        } else {
                            QMessageBox::warning(this, "Błąd", "Baza odrzuciła kategorię.");
                        }
                    }
                }
            });
        }
        else if (trybGrupowania == 2) { // Tryb Platformy
            menu.addAction("Dodaj nową platformę...", this, [this]() {
                bool ok;
                QString nazwa = QInputDialog::getText(this, "Nowa Platforma", "Podaj nazwę:", QLineEdit::Normal, "", &ok);
                if (ok && !nazwa.trimmed().isEmpty()) {
                    if (dbManager.dodajPlatforme(nazwa.trimmed()) > 0) {
                        zaladujDaneDoDrzewa();
                        uzupelnijComboBoxy();
                    }
                }
            });
        }
    }
    else if (kliknietyElement->parent() == nullptr) {
        // 2. KLIKNIĘTO W KORZEŃ (Grupę)
        int trybGrupowania = ui->comboGrupowanie->currentIndex();

        if (trybGrupowania == 0) {
            // WIDOK KATEGORII
            menu.addAction("Dodaj pozycję do tej kategorii", this, [this, kliknietyElement]() {
                int idKategorii = kliknietyElement->data(0, Qt::UserRole).toInt();
                przygotujFormularz(-1, idKategorii, 0);
            });

            if (kliknietyElement->text(0) != "Brak kategorii") {
                menu.addAction("Edytuj kategorię...", this, [this, kliknietyElement]() {
                    int idKat = kliknietyElement->data(0, Qt::UserRole).toInt();
                    QString obecnaNazwa = kliknietyElement->text(0);
                    QString obecnaJednostka = kliknietyElement->data(0, Qt::UserRole + 1).toString();

                    QDialog dialog(this);
                    dialog.setWindowTitle("Edytuj Kategorię");
                    dialog.resize(300, 100);

                    QFormLayout form(&dialog);
                    QLineEdit editNazwa(&dialog);
                    editNazwa.setText(obecnaNazwa); // Wypełniamy obecną nazwą!

                    QComboBox comboJednostka(&dialog);
                    comboJednostka.setEditable(true);
                    comboJednostka.addItems(dbManager.pobierzUnikalneJednostki());
                    comboJednostka.setCurrentText(obecnaJednostka); // Ustawiamy obecną jednostkę!

                    form.addRow("Nazwa kategorii:", &editNazwa);
                    form.addRow("Jednostka:", &comboJednostka);

                    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
                    form.addRow(&buttonBox);

                    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
                    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

                    if (dialog.exec() == QDialog::Accepted) {
                        QString nowaNazwa = editNazwa.text().trimmed();
                        QString nowaJednostka = comboJednostka.currentText().trimmed();

                        if (!nowaNazwa.isEmpty()) {
                            if (dbManager.aktualizujKategorie(idKat, nowaNazwa, nowaJednostka)) {
                                // PRZEŁADOWUJEMY Z BAZY - bo zmiana jednostki wpływa na wszystkie multimedia w tej kategorii
                                listaMultimediow = dbManager.getAllMultimedia();
                                zaladujDaneDoDrzewa();
                                uzupelnijComboBoxy();
                                ui->statusbar->showMessage("Kategoria zaktualizowana!", 3000);
                            }
                        }
                    }
                });

                menu.addSeparator();

                menu.addAction("Usuń kategorię...", this, [this, kliknietyElement]() {
                    int idKat = kliknietyElement->data(0, Qt::UserRole).toInt();
                    QString nazwa = kliknietyElement->text(0);

                    QMessageBox msgBox(this);
                    msgBox.setWindowTitle("Usuwanie kategorii");
                    msgBox.setText("Usuwasz kategorię: " + nazwa + "\nCo chcesz zrobić z przypisanymi do niej pozycjami?");

                    // Dodajemy własne przyciski
                    QPushButton *btnPrzenies = msgBox.addButton("Przenieś do 'Brak kategorii'", QMessageBox::ActionRole);
                    QPushButton *btnUsunWszystko = msgBox.addButton("Usuń kategorię i WSZYSTKIE jej pozycje", QMessageBox::DestructiveRole);
                    QPushButton *btnAnuluj = msgBox.addButton("Anuluj", QMessageBox::RejectRole);

                    msgBox.exec();

                    if (msgBox.clickedButton() == btnAnuluj) return; // Użytkownik stchórzył

                    bool usunPowiazane = (msgBox.clickedButton() == btnUsunWszystko);

                    if (dbManager.usunKategorie(idKat, usunPowiazane)) {
                        listaMultimediow = dbManager.getAllMultimedia(); // Pełne przeładowanie bo usunęliśmy elementy
                        zaladujDaneDoDrzewa();
                        uzupelnijComboBoxy();
                        odswiezStatystykiGlowne();
                        ui->statusbar->showMessage("Kategoria została usunięta.", 4000);
                    } else {
                        QMessageBox::critical(this, "Błąd", "Nie udało się usunąć kategorii. Baza odrzuciła transakcję.");
                    }
                });
            }
        }
        else if (trybGrupowania == 2) {
            // WIDOK PLATFORMY
            menu.addAction("Dodaj pozycję do tej platformy", this, [this, kliknietyElement]() {
                int idPlatformy = kliknietyElement->data(0, Qt::UserRole).toInt();
                przygotujFormularz(-1, 0, idPlatformy);
            });

            // Zabezpieczenie przed psuciem bazy i usuwaniem "Nieznanej platformy"
            if (kliknietyElement->text(0) != "Nieznana platforma") {
                menu.addAction("Zmień nazwę platformy", this, [kliknietyElement]() {
                    qDebug() << "Zmieniam nazwę platformy: " << kliknietyElement->text(0);
                    // TODO: Dodać okienko z QInputDialog i zapytanie UPDATE do bazy
                });

                menu.addSeparator();

                menu.addAction("Usuń platformę...", this, [this, kliknietyElement]() {
                    int idPlat = kliknietyElement->data(0, Qt::UserRole).toInt();
                    QString nazwa = kliknietyElement->text(0);

                    QMessageBox msgBox(this);
                    msgBox.setWindowTitle("Usuwanie platformy");
                    msgBox.setText("Usuwasz platformę: " + nazwa + "\nCo chcesz zrobić z przypisanymi do niej pozycjami?");

                    QPushButton *btnPrzenies = msgBox.addButton("Przenieś do 'Nieznana platforma'", QMessageBox::ActionRole);
                    QPushButton *btnUsunWszystko = msgBox.addButton("Usuń platformę i WSZYSTKIE jej pozycje", QMessageBox::DestructiveRole);
                    QPushButton *btnAnuluj = msgBox.addButton("Anuluj", QMessageBox::RejectRole);

                    msgBox.exec();

                    if (msgBox.clickedButton() == btnAnuluj) return;

                    bool usunPowiazane = (msgBox.clickedButton() == btnUsunWszystko);

                    if (dbManager.usunPlatforme(idPlat, usunPowiazane)) {
                        listaMultimediow = dbManager.getAllMultimedia();
                        zaladujDaneDoDrzewa();
                        uzupelnijComboBoxy();
                        odswiezStatystykiGlowne();
                        ui->statusbar->showMessage("Platforma została usunięta.", 4000);
                    } else {
                        QMessageBox::critical(this, "Błąd", "Nie udało się usunąć platformy. Baza odrzuciła transakcję.");
                    }
                });
            }
        }
    }
    else {
        // 3. KLIKNIĘTO W KONKRETNE MEDIUM (np. "Cyberpunk 2077")
        menu.addAction("Edytuj pozycję", this, [this, kliknietyElement]() {
            int idMedium = kliknietyElement->data(0, Qt::UserRole).toInt();
            przygotujFormularz(idMedium, 0); // Tryb edycji na podstawie ID!
        });

        // --- NOWOŚĆ: Skrót dla leniwych (Szybka zmiana kategorii jednego elementu) ---
        int trybGrupowania = ui->comboGrupowanie->currentIndex();
        if (trybGrupowania == 0) { // Pokazujemy ten skrót TYLKO, gdy drzewo jest pogrupowane po kategoriach
            menu.addAction("Zmień kategorię", this, [this, kliknietyElement]() {
                int idMedium = kliknietyElement->data(0, Qt::UserRole).toInt();
                int obecneIdKategorii = kliknietyElement->parent()->data(0, Qt::UserRole).toInt();

                QDialog dialog(this);
                dialog.setWindowTitle("Zmiana kategorii");
                dialog.resize(300, 100);

                QFormLayout form(&dialog);
                QComboBox comboNowaKat(&dialog);

                // Ładujemy kategorie z bazy
                auto kategorie = dbManager.pobierzKategorie();
                for (const auto& kat : std::as_const(kategorie)) {
                    if (kat.first != 0) comboNowaKat.addItem(kat.second, kat.first);
                }

                // Magia UX: Ustawiamy domyślnie tę kategorię, w której aktualnie jest gra!
                int index = comboNowaKat.findData(obecneIdKategorii);
                if (index != -1) comboNowaKat.setCurrentIndex(index);

                form.addRow("Przenieś do:", &comboNowaKat);
                QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
                form.addRow(&buttonBox);

                connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
                connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

                if (dialog.exec() == QDialog::Accepted) {
                    int noweIdKat = comboNowaKat.currentData().toInt();

                    // Sprawdzamy, czy użytkownik faktycznie zmienił opcję, czy po prostu kliknął OK z nudów
                    if (noweIdKat != obecneIdKategorii) {
                        // Wywołujemy naszą zbiorczą funkcję z poprzedniego punktu, podając listę z 1 elementem!
                        if (dbManager.zmienKategorieWielu({idMedium}, noweIdKat)) {
                            listaMultimediow = dbManager.getAllMultimedia();
                            zaladujDaneDoDrzewa();
                            ui->statusbar->showMessage("Kategoria zmieniona w mgnieniu oka.", 3000);
                        } else {
                            QMessageBox::critical(this, "Błąd", "Baza danych stawiła opór.");
                        }
                    }
                }
            });
        }
        // -----------------------------------------------------------------------------

        menu.addSeparator();
        // Fragment wewnątrz pokazMenuDrzewa dla konkretnego medium:
        menu.addAction("Usuń z biblioteki", this, [this, kliknietyElement]() {
            int id = kliknietyElement->data(0, Qt::UserRole).toInt();

            if (QMessageBox::question(this, "Potwierdzenie", "Czy na pewno usunąć?") == QMessageBox::Yes) {
                if (dbManager.usunMedium(id)) {
                    listaMultimediow = dbManager.getAllMultimedia();
                    zaladujDaneDoDrzewa();
                    odswiezStatystykiGlowne();
                    ui->daneSzczegolowe->setCurrentIndex(0);
                }
            }
        });
    }

    // Wyświetlamy menu w miejscu kursora myszy
    menu.exec(ui->kategorie->mapToGlobal(pos));
}

