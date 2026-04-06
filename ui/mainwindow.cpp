#include "mainwindow.h"
#include "ui_mainwindow.h"
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->comboDetaleStatus->addItem("Planowane");
    ui->comboDetaleStatus->addItem("W trakcie");
    ui->comboDetaleStatus->addItem("Ukończone");
    ui->comboDetaleStatus->addItem("Porzucone");
    ui->comboGrupowanie->addItems({"Kategoria", "Status", "Platforma", "Data dodania"});

    ui->kategorie->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->kategorie->setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(ui->comboGrupowanie, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        zaladujDaneDoDrzewa();
    });

    connect(ui->kategorie, &QTreeWidget::itemClicked,
            this, &MainWindow::onWybieranieElementuDrzewa);

    connect(ui->wyszukiwarka, &QLineEdit::textChanged,
            this, &MainWindow::onWyszukiwanie);

    connect(ui->spinDetaleAktualny, &QSpinBox::valueChanged, this, [this](int wartosc) {
        // Jeśli osiągnięto cel i cel jest większy od zera (żeby nie kończyło pustych gier)
        if (wartosc > 0 && wartosc >= ui->spinDetaleDocelowy->value()) {
            ui->comboDetaleStatus->setCurrentText("Ukończone");
        }
    });

    // --- AKCJA: ZAPISZ ZMIANY ---
    connect(ui->btnDetaleZapisz, &QPushButton::clicked, this, [this]() {
        auto wybrane = ui->kategorie->selectedItems();
        if (wybrane.isEmpty()) return; // Zabezpieczenie: nic nie jest kliknięte

        // Pobieramy ID ukryte w klikniętym elemencie drzewa
        int id = wybrane.first()->data(0, Qt::UserRole).toInt();

        // Pobieramy wartości z naszych kontrolek
        QString nowyStatus = ui->comboDetaleStatus->currentText();
        int nowaAktualna = ui->spinDetaleAktualny->value();
        int nowaDocelowa = ui->spinDetaleDocelowy->value();

        // Wysyłamy do bazy
        if (dbManager.aktualizujPostep(id, nowyStatus, nowaAktualna, nowaDocelowa)) {

            // Zaktualizowano w bazie, więc uaktualniamy też naszą listę w pamięci
            // (żeby nie musieć pobierać wszystkiego z bazy od nowa)
            for(auto& medium : listaMultimediow) {
                if(medium->getId() == id) {
                    medium->setStatus(nowyStatus);
                    Postep p = medium->getPostep();
                    p.aktualna = nowaAktualna;
                    p.docelowa = nowaDocelowa;
                    medium->setPostep(p);
                    break;
                }
            }

            // Odświeżamy ten sam widok, żeby ukryć kontrolki jeśli status to "Ukończone"
            onWybieranieElementuDrzewa(wybrane.first(), 0);

            // Odświeżamy statystyki na głównej stronie!
            odswiezStatystykiGlowne();

            // Wyświetlamy mały komunikat na pasku na dole na 3 sekundy
            ui->statusbar->showMessage("Zapisano zmiany w bazie!", 3000);
        }
    });

    // --- AKCJA: ZACZNIJ OD NOWA ---
    connect(ui->btnZacznijOdNowa, &QPushButton::clicked, this, [this]() {
        auto wybrane = ui->kategorie->selectedItems();
        if (wybrane.isEmpty()) return;

        int id = wybrane.first()->data(0, Qt::UserRole).toInt();

        if (dbManager.zacznijOdNowa(id)) {
            for(auto& medium : listaMultimediow) {
                if(medium->getId() == id) {
                    medium->setStatus("W trakcie");
                    Postep p = medium->getPostep();
                    p.aktualna = 0; // W pamięci też zerujemy!
                    medium->setPostep(p);
                    break;
                }
            }
            // Odświeżamy widok - zielony napis zniknie, pojawią się znów okienka
            onWybieranieElementuDrzewa(wybrane.first(), 0);
            odswiezStatystykiGlowne();
            ui->statusbar->showMessage("Licznik wyzerowany. Lecimy od nowa!", 3000);
        }
    });

    connect(ui->spinDetaleAktualny, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        if (val > 0 && ui->comboDetaleStatus->currentText() == "Planowane") {
            ui->comboDetaleStatus->setCurrentText("W trakcie");
        }
    });

    connect(ui->spinDetaleDocelowy, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int maxVal) {
        ui->spinDetaleAktualny->setMaximum(maxVal);
    });

    connect(ui->kategorie, &QTreeWidget::customContextMenuRequested, this, &MainWindow::pokazMenuDrzewa);

    connect(ui->btnPotwierdzDodaj, &QPushButton::clicked, this, [this]() {
        QString tytul = ui->editNowyTytul->text().trimmed();
        if (tytul.isEmpty()) {
            QMessageBox::warning(this, "Błąd", "Tytuł nie może być pusty!");
            return;
        }

        // Pobieramy ID z ukrytych danych Comboboxa, a nie tekst!
        int idKat = ui->comboNowaKategoria->currentData().toInt();
        int idPlat = ui->comboNowyTyp->currentData().toInt();
        int cel = ui->spinNowyCel->value();

        if (idKat == 0) {
            QMessageBox::warning(this, "Brak kategorii", "Wybierz kategorię z listy!");
            return;
        }

        bool sukces = false;
        if (czyTrybEdycji) {
            sukces = dbManager.aktualizujDaneMedium(idEdytowanegoMedium, tytul, idKat, idPlat, cel);
            if (sukces) {
                // Zmieniamy kolor na zielony i wrzucamy tekst
                ui->lblKomunikatFormularza->setStyleSheet("color: green; font-weight: bold;");
                ui->lblKomunikatFormularza->setText("Zaktualizowano: " + tytul);
            }
        } else {
            sukces = dbManager.dodajNoweMedium(tytul, idKat, idPlat, cel);
            if (sukces) {
                ui->lblKomunikatFormularza->setStyleSheet("color: green; font-weight: bold;");
                ui->lblKomunikatFormularza->setText("Dodano do biblioteki: "+ tytul+"!");
                ui->editNowyTytul->clear();
            }
        }

        if (sukces) {
            listaMultimediow = dbManager.getAllMultimedia();
            zaladujDaneDoDrzewa();
            odswiezStatystykiGlowne();
            uzupelnijComboBoxy();

            // Odpalamy zegarek, który po 4000 ms (4 sekundy) wyczyści ten napis
            QTimer::singleShot(4000, this, [this]() {
                ui->lblKomunikatFormularza->clear();
            });

        } else {
            ui->lblKomunikatFormularza->setStyleSheet("color: red; font-weight: bold;");
            ui->lblKomunikatFormularza->setText("Błąd bazy danych!");
            QTimer::singleShot(4000, this, [this]() { ui->lblKomunikatFormularza->clear(); });
        }
    });

    connect(ui->btnDodajMedium, &QPushButton::clicked, this, [this]() {
        przygotujFormularz(-1, 0); // Otwiera w trybie dodawania, kategoria 0
    });

    connect(ui->btnAnulujDodawanie, &QPushButton::clicked, this, [this]() {
        czyTrybEdycji = false;
        idEdytowanegoMedium = -1;
        ui->daneSzczegolowe->setCurrentWidget(ui->page); // Uciekamy bez zapisu
    });

    connect(ui->btnDodajPlatforme, &QPushButton::clicked, this, [this]() {
        bool ok;
        // Otwiera małe, systemowe okienko z pytaniem o tekst
        QString nowaNazwa = QInputDialog::getText(this, "Nowa Platforma",
                                                  "Podaj nazwę nowej platformy:",
                                                  QLineEdit::Normal, "", &ok);

        nowaNazwa = nowaNazwa.trimmed();
        if (ok && !nowaNazwa.isEmpty()) {
            // Tu dodajesz do bazy nową platformę (musisz dopisać prosty INSERT w DatabaseManager)
            int noweId = dbManager.dodajPlatforme(nowaNazwa);

            if (noweId > 0) {
                // Odświeżasz listy rozwijane
                uzupelnijComboBoxy();

                // Magia UX: Automatycznie wybierasz tę nową platformę na liście!
                int index = ui->comboNowyTyp->findData(noweId);
                if (index != -1) {
                    ui->comboNowyTyp->setCurrentIndex(index);
                }
                ui->statusbar->showMessage("Dodano i wybrano nową platformę", 3000);
            } else {
                QMessageBox::warning(this, "Błąd", "Nie udało się dodać platformy do bazy.");
            }
        }
    });

    connect(ui->btnDodajKategorie, &QPushButton::clicked, this, [this]() {
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
                    // zaladujDaneDoDrzewa();

                    ui->statusbar->showMessage("Dodano kategorię!", 3000);
                } else {
                    QMessageBox::warning(this, "Błąd", "Baza odrzuciła kategorię.");
                }
            }
        }
    });

    connect(ui->comboNowaKategoria, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index >= 0) {
            // Wyciągamy naszą schowaną jednostkę z wybranego elementu
            QString jednostka = ui->comboNowaKategoria->itemData(index, Qt::UserRole + 1).toString();

            // Zabezpieczenie przed pustym ciągiem znaków
            ui->labJednostka->setText(jednostka.isEmpty() ? "Cel:" : "Cel (" + jednostka + "):");
        }
    });

    connect(ui->actionKategorie, &QAction::triggered, this, [this]() {
        QDialog dialog(this);
        dialog.setWindowTitle("Zarządzanie Kategoriami");
        dialog.resize(500, 400);

        QVBoxLayout layout(&dialog);

        // Tworzymy tabelę
        QTableWidget tabela(0, 3, &dialog);
        tabela.setHorizontalHeaderLabels({"ID", "Nazwa", "Jednostka"});
        tabela.setSelectionBehavior(QAbstractItemView::SelectRows); // Zaznaczamy całe wiersze
        tabela.setSelectionMode(QAbstractItemView::SingleSelection); // Tylko jeden wiersz na raz
        tabela.setEditTriggers(QAbstractItemView::NoEditTriggers); // Blokada edycji przez dwuklik (mamy od tego swój przycisk)
        tabela.horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); // Nazwa zajmuje resztę miejsca
        tabela.hideColumn(0); // Ukrywamy kolumnę z ID, bo użytkownika to nie obchodzi

        // Wypełniamy tabelę danymi z bazy
        auto wypelnijTabele = [&]() {
            tabela.setRowCount(0); // Czyszczenie
            QSqlQuery q("SELECT id, nazwa, jednostka FROM kategorie ORDER BY nazwa");
            int wiersz = 0;
            while(q.next()) {
                tabela.insertRow(wiersz);
                tabela.setItem(wiersz, 0, new QTableWidgetItem(q.value(0).toString()));
                tabela.setItem(wiersz, 1, new QTableWidgetItem(q.value(1).toString()));
                tabela.setItem(wiersz, 2, new QTableWidgetItem(q.value(2).toString()));
                wiersz++;
            }
        };
        wypelnijTabele();

        layout.addWidget(&tabela);

        // Panel przycisków na dole
        QHBoxLayout btnLayout;
        QPushButton btnDodaj("Dodaj nową", &dialog);
        QPushButton btnEdytuj("Edytuj wybraną", &dialog);
        QPushButton btnUsun("Usuń wybraną", &dialog);
        QPushButton btnZamknij("Zamknij", &dialog);

        btnLayout.addWidget(&btnDodaj);
        btnLayout.addWidget(&btnEdytuj);
        btnLayout.addWidget(&btnUsun);
        btnLayout.addStretch();
        btnLayout.addWidget(&btnZamknij);

        layout.addLayout(&btnLayout);

        // --- AKCJE PRZYCISKÓW ---

        connect(&btnZamknij, &QPushButton::clicked, &dialog, &QDialog::accept);

        // DODAWANIE (Wywołuje ten sam kod, co przycisk +)
        connect(&btnDodaj, &QPushButton::clicked, this, [&]() {
            ui->btnDodajKategorie->click(); // Sprytny trick: symulujemy kliknięcie w istniejący przycisk szybkiego dodawania!
            wypelnijTabele(); // Odświeżamy tabelę w oknie
        });

        // EDYCJA
        connect(&btnEdytuj, &QPushButton::clicked, this, [&]() {
            int wiersz = tabela.currentRow();
            if (wiersz < 0) {
                QMessageBox::warning(&dialog, "Brak wyboru", "Zaznacz najpierw kategorię na liście.");
                return;
            }

            // Pobieramy dane z tabeli
            int idKat = tabela.item(wiersz, 0)->text().toInt();
            QString obecnaNazwa = tabela.item(wiersz, 1)->text();
            QString obecnaJednostka = tabela.item(wiersz, 2)->text();

            // Odpalamy okienko edycji (podobne do tego, co masz w menu kontekstowym)
            QDialog editDialog(&dialog);
            editDialog.setWindowTitle("Edytuj: " + obecnaNazwa);
            QFormLayout form(&editDialog);
            QLineEdit editNazwa(&editDialog);
            editNazwa.setText(obecnaNazwa);
            QComboBox comboJednostka(&editDialog);
            comboJednostka.setEditable(true);
            comboJednostka.addItems(dbManager.pobierzUnikalneJednostki());
            comboJednostka.setCurrentText(obecnaJednostka);

            form.addRow("Nazwa:", &editNazwa);
            form.addRow("Jednostka:", &comboJednostka);
            QDialogButtonBox bb(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &editDialog);
            form.addRow(&bb);
            connect(&bb, &QDialogButtonBox::accepted, &editDialog, &QDialog::accept);
            connect(&bb, &QDialogButtonBox::rejected, &editDialog, &QDialog::reject);

            if (editDialog.exec() == QDialog::Accepted && !editNazwa.text().trimmed().isEmpty()) {
                if (dbManager.aktualizujKategorie(idKat, editNazwa.text().trimmed(), comboJednostka.currentText().trimmed())) {
                    wypelnijTabele();
                }
            }
        });

        connect(ui->actionPlatformy, &QAction::triggered, this, [this]() {
            QDialog dialog(this);
            dialog.setWindowTitle("Zarządzanie Platformami");
            dialog.resize(400, 400);

            QVBoxLayout layout(&dialog);

            QTableWidget tabela(0, 2, &dialog);
            tabela.setHorizontalHeaderLabels({"ID", "Nazwa"});
            tabela.setSelectionBehavior(QAbstractItemView::SelectRows);
            tabela.setSelectionMode(QAbstractItemView::SingleSelection);
            tabela.setEditTriggers(QAbstractItemView::NoEditTriggers);
            tabela.horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
            tabela.hideColumn(0);

            auto wypelnijTabele = [&]() {
                tabela.setRowCount(0);
                QSqlQuery q("SELECT id, nazwa FROM platformy ORDER BY nazwa");
                int wiersz = 0;
                while(q.next()) {
                    tabela.insertRow(wiersz);
                    tabela.setItem(wiersz, 0, new QTableWidgetItem(q.value(0).toString()));
                    tabela.setItem(wiersz, 1, new QTableWidgetItem(q.value(1).toString()));
                    wiersz++;
                }
            };
            wypelnijTabele();

            layout.addWidget(&tabela);

            QHBoxLayout btnLayout;
            QPushButton btnDodaj("Dodaj nową", &dialog);
            QPushButton btnEdytuj("Edytuj wybraną", &dialog);
            QPushButton btnUsun("Usuń wybraną", &dialog);
            QPushButton btnZamknij("Zamknij", &dialog);

            btnLayout.addWidget(&btnDodaj);
            btnLayout.addWidget(&btnEdytuj);
            btnLayout.addWidget(&btnUsun);
            btnLayout.addStretch();
            btnLayout.addWidget(&btnZamknij);
            layout.addLayout(&btnLayout);

            connect(&btnZamknij, &QPushButton::clicked, &dialog, &QDialog::accept);

            connect(&btnDodaj, &QPushButton::clicked, this, [&]() {
                ui->btnDodajPlatforme->click();
                wypelnijTabele();
            });

            connect(&btnEdytuj, &QPushButton::clicked, this, [&]() {
                int wiersz = tabela.currentRow();
                if (wiersz < 0) return;

                int idPlat = tabela.item(wiersz, 0)->text().toInt();
                QString obecnaNazwa = tabela.item(wiersz, 1)->text();

                bool ok;
                QString nowaNazwa = QInputDialog::getText(&dialog, "Edytuj Platformę", "Nazwa:", QLineEdit::Normal, obecnaNazwa, &ok);
                if (ok && !nowaNazwa.trimmed().isEmpty()) {
                    if (dbManager.aktualizujPlatforme(idPlat, nowaNazwa.trimmed())) {
                        wypelnijTabele();
                    }
                }
            });

            connect(&btnUsun, &QPushButton::clicked, this, [&]() {
                int wiersz = tabela.currentRow();
                if (wiersz < 0) return;

                int idPlat = tabela.item(wiersz, 0)->text().toInt();
                QString nazwa = tabela.item(wiersz, 1)->text();

                QMessageBox msgBox(&dialog);
                msgBox.setWindowTitle("Usuwanie");
                msgBox.setText("Usuwasz platformę: " + nazwa + "\nCo robimy z zawartością?");
                QPushButton *btnPrzenies = msgBox.addButton("Przenieś do 'Nieznana'", QMessageBox::ActionRole);
                QPushButton *btnUsunWszystko = msgBox.addButton("Usuń wszystko", QMessageBox::DestructiveRole);
                QPushButton *btnAnuluj = msgBox.addButton("Anuluj", QMessageBox::RejectRole);

                msgBox.exec();
                if (msgBox.clickedButton() == btnAnuluj) return;

                bool usunZDetalami = (msgBox.clickedButton() == btnUsunWszystko);
                if (dbManager.usunPlatforme(idPlat, usunZDetalami)) {
                    wypelnijTabele();
                }
            });

            dialog.exec();

            // Przeładowanie globalne po zamknięciu okna
            listaMultimediow = dbManager.getAllMultimedia();
            zaladujDaneDoDrzewa();
            uzupelnijComboBoxy();
        });

        // USUWANIE (Reużywamy Twoją potężną funkcję z transakcjami)
        connect(&btnUsun, &QPushButton::clicked, this, [&]() {
            int wiersz = tabela.currentRow();
            if (wiersz < 0) return;

            int idKat = tabela.item(wiersz, 0)->text().toInt();
            QString nazwa = tabela.item(wiersz, 1)->text();

            QMessageBox msgBox(&dialog);
            msgBox.setWindowTitle("Usuwanie");
            msgBox.setText("Usuwasz kategorię: " + nazwa + "\nCo robimy z zawartością?");
            QPushButton *btnPrzenies = msgBox.addButton("Przenieś do 'Brak'", QMessageBox::ActionRole);
            QPushButton *btnUsunWszystko = msgBox.addButton("Usuń wszystko", QMessageBox::DestructiveRole);
            QPushButton *btnAnuluj = msgBox.addButton("Anuluj", QMessageBox::RejectRole);

            msgBox.exec();
            if (msgBox.clickedButton() == btnAnuluj) return;

            bool usunZDetalami = (msgBox.clickedButton() == btnUsunWszystko);
            if (dbManager.usunKategorie(idKat, usunZDetalami)) {
                wypelnijTabele();
            }
        });

        // Odpalamy okno modalnie
        dialog.exec();

        // Gdy okno zostanie zamknięte, odświeżamy cały program, bo słowniki mogły ulec zmianie
        listaMultimediow = dbManager.getAllMultimedia();
        zaladujDaneDoDrzewa();
        uzupelnijComboBoxy();
    });

    connect(ui->btnPowrot, &QPushButton::clicked, this, [this]() {
        // Wracamy na start i odświeżamy widok, żeby uwzględnić ewentualne zmiany
        odswiezStatystykiGlowne();
        ui->daneSzczegolowe->setCurrentWidget(ui->page);
    });

    connect(ui->btnLosuj, &QPushButton::clicked, this, [this]() {
        QList<int> doWylosowania;

        // Zbieramy ID tylko tych gier/książek, które nie są ruszone ("Planowane")
        for (const auto& m : std::as_const(listaMultimediow)) {
            if (m->getStatus() == "Planowane") {
                doWylosowania.append(m->getId());
            }
        }

        if (doWylosowania.isEmpty()) {
            QMessageBox::information(this, "Pusto!", "Nie masz żadnych 'Planowanych' pozycji w bibliotece. Dodaj coś nowego!");
            return;
        }

        // Losujemy indeks z listy
        int wylosowanyIndeks = QRandomGenerator::global()->bounded(doWylosowania.size());
        int wylosowaneId = doWylosowania.at(wylosowanyIndeks);

        // Otwieramy detale za pomocą naszej uniwersalnej funkcji
        pokazSzczegolyMedium(wylosowaneId);

        // Możemy dorzucić toast na pasku, żeby było bardziej interaktywnie
        ui->statusbar->showMessage("Oto twoje wylosowane przeznaczenie!", 4000);
    });

    connect(ui->btnPowrot, &QPushButton::clicked, this, [this]() {
        ui->daneSzczegolowe->setCurrentWidget(ui->page); // Powrót na start
        odswiezStatystykiGlowne(); // Odświeżamy wykres i ostatnie pozycje
    });

    if (dbManager.openConnection()) {
        zaladujDaneDoDrzewa();
        odswiezStatystykiGlowne();
        uzupelnijComboBoxy();
        ui->daneSzczegolowe->setCurrentIndex(0);
    } else {
        qDebug() << "Baza leży i kwiczy, nie ładuję drzewa.";
    }
}

MainWindow::~MainWindow()
{
    listaMultimediow.clear();
    delete ui;
}

void MainWindow::onWybieranieElementuDrzewa(QTreeWidgetItem *item, int column) {
    if (!item || item->parent() == nullptr) {
        if (item) item->setExpanded(!item->isExpanded());
        odswiezStatystykiGlowne();
        ui->daneSzczegolowe->setCurrentWidget(ui->page);
        return;
    }
    // Wyciągamy ID i odpalamy naszą nową, uniwersalną funkcję
    int idWybranegoElementu = item->data(0, Qt::UserRole).toInt();
    pokazSzczegolyMedium(idWybranegoElementu);
}

void MainWindow::onWyszukiwanie(const QString &text) {
    // Zabezpieczenie: jeśli drzewo jest puste, nie ma co szukać
    if (ui->kategorie->topLevelItemCount() == 0) return;

    // Przechodzimy pętlą przez wszystkie główne kategorie ("Gry", "Książki", itp.)
    for (int i = 0; i < ui->kategorie->topLevelItemCount(); ++i) {
        QTreeWidgetItem *kategoria = ui->kategorie->topLevelItem(i);
        bool maPasujaceElementy = false;

        // Druga pętla: Przechodzimy przez wszystkie tytuły wewnątrz danej kategorii
        for (int j = 0; j < kategoria->childCount(); ++j) {
            QTreeWidgetItem *dziecko = kategoria->child(j);

            // Sprawdzamy czy tytuł zawiera szukany tekst (Qt::CaseInsensitive ignoruje wielkość liter!)
            if (dziecko->text(0).contains(text, Qt::CaseInsensitive)) {
                dziecko->setHidden(false); // Pokazujemy pasujący
                maPasujaceElementy = true; // Zaznaczamy, że kategoria ma chociaż jednego potomka
            } else {
                dziecko->setHidden(true);  // Ukrywamy niepasujący
            }
        }

        // Magia interfejsu (UX)
        if (text.isEmpty()) {
            // Jeśli pasek wyszukiwania jest pusty, przywracamy widoczność wszystkich kategorii
            kategoria->setHidden(false);
            // Kategoria wraca do stanu domyślnego - możemy ją np. zwinąć, żeby odgruzować widok
            // kategoria->setExpanded(false);
        } else {
            // Jeśli szukamy konkretnego tekstu:
            // Ukrywamy kategorię, jeśli nic w niej nie znaleziono
            kategoria->setHidden(!maPasujaceElementy);

            // Jeśli coś znaleziono, wymuszamy rozwinięcie kategorii, żeby wynik był widoczny
            if (maPasujaceElementy) {
                kategoria->setExpanded(true);
            }
        }
    }
}

void MainWindow::zaladujDaneDoDrzewa() {
    ui->kategorie->clear();

    // Jeśli lista w pamięci jest pusta (np. po starcie), pobierz z bazy
    if (listaMultimediow.isEmpty()) {
        listaMultimediow = dbManager.getAllMultimedia();
    }

    int trybGrupowania = ui->comboGrupowanie->currentIndex();

    // Potrzebujemy słowników pomocniczych do mapowania ID na nazwy,
    // jeśli grupujemy po encjach z bazy.
    QMap<int, QString> slownikKategorii;
    QList<QPair<int, QString>> listaPlatform;
    if (trybGrupowania == 0) slownikKategorii = dbManager.getCategories();
    if (trybGrupowania == 2) listaPlatform = dbManager.pobierzPlatformy();

    // Mapa, w której kluczem jest nazwa głównej gałęzi, a wartością wskaźnik na nią w UI
    QMap<QString, QTreeWidgetItem*> wezlyGlowne;

    if (trybGrupowania == 0) { // Tryb Kategorii
        QSqlQuery q("SELECT id, nazwa, jednostka FROM kategorie");
        while(q.next()) {
            QString nazwa = q.value(1).toString();
            QTreeWidgetItem *wezel = new QTreeWidgetItem(ui->kategorie);
            wezel->setText(0, nazwa);
            // Zapisujemy ID i jednostkę dla menu edycji
            wezel->setData(0, Qt::UserRole, q.value(0).toInt());
            wezel->setData(0, Qt::UserRole + 1, q.value(2).toString());
            wezlyGlowne.insert(nazwa, wezel);
        }
    } else if (trybGrupowania == 2) { // Tryb Platform
        for (const auto& plat : listaPlatform) {
            QString nazwa = plat.second;
            QTreeWidgetItem *wezel = new QTreeWidgetItem(ui->kategorie);
            wezel->setText(0, nazwa);
            wezel->setData(0, Qt::UserRole, plat.first); // Zapisujemy ID platformy
            wezlyGlowne.insert(nazwa, wezel);
        }
    }

    // Przechodzimy przez wszystkie multimedia i dynamicznie szukamy im miejsca
    for (const auto& medium : std::as_const(listaMultimediow)) {
        QString nazwaGrupy;

        // Decydujemy, jaka jest etykieta dla grupy
        switch (trybGrupowania) {
        case 0: // Kategoria
            nazwaGrupy = slownikKategorii.value(medium->getIdKategorii(), "Brak kategorii");
            break;
        case 1: // Status
            nazwaGrupy = medium->getStatus();
            break;
        case 2: { // Platforma
            // Szukamy nazwy platformy po ID
            nazwaGrupy = "Nieznana platforma";
            for (const auto& plat : listaPlatform) {
                if (plat.first == medium->getIdPlatformy()) {
                    nazwaGrupy = plat.second;
                    break;
                }
            }
            break;
        }
        case 3: // Data dodania
            QLocale polski(QLocale::Polish, QLocale::Poland);
            QDate data = medium->getDataDodania().date();

            if (data.isValid()) {
                // standaloneMonthName wyciąga mianownik: Styczeń, Luty, Kwiecień itd.
                QString miesiac = polski.standaloneMonthName(data.month());

                // Łączymy miesiąc z rokiem i rzucamy na wielkie litery
                nazwaGrupy = QString("%1 %2").arg(miesiac).arg(data.year()).toUpper();
            } else {
                nazwaGrupy = "BRAK DATY";
            }
            break;
        }

        // Jeśli taki wezeł jeszcze nie istnieje, tworzymy go
        if (!wezlyGlowne.contains(nazwaGrupy)) {
            QTreeWidgetItem *nowyWezel = new QTreeWidgetItem(ui->kategorie);
            nowyWezel->setText(0, nazwaGrupy);

            // Zapisujemy dodatkowe dane dla menu kontekstowego zależnie od trybu
            if (trybGrupowania == 0) {
                nowyWezel->setData(0, Qt::UserRole, medium->getIdKategorii());
                nowyWezel->setData(0, Qt::UserRole + 1, medium->getPostep().jednostka);
            } else if (trybGrupowania == 2) {
                nowyWezel->setData(0, Qt::UserRole, medium->getIdPlatformy());
            }

            wezlyGlowne.insert(nazwaGrupy, nowyWezel);
        }

        // Przypinamy pozycję (dziecko) do odpowiedniej grupy
        QTreeWidgetItem *item = new QTreeWidgetItem(wezlyGlowne[nazwaGrupy]);
        item->setText(0, medium->getTytul());
        item->setData(0, Qt::UserRole, medium->getId());
        item->setData(0, Qt::UserRole + 1, medium->getPostep().jednostka);
    }

    ui->kategorie->expandAll();
}


void MainWindow::odswiezStatystykiGlowne() {
    auto stats = dbManager.getGlobalStats();

    // Zabezpieczenie: Jeśli nie ma nic w bazie, nie rysujemy pustego wykresu
    if (stats.value("Suma", 0) == 0) return;

    // 2. Budujemy serię danych do wykresu kołowego
    QPieSeries *series = new QPieSeries();
    series->append("Planowane", stats.value("Planowane", 0));
    series->append("W trakcie", stats.value("W trakcie", 0));
    series->append("Ukończone", stats.value("Ukończone", 0));
    series->append("Porzucone", stats.value("Porzucone", 0));

    // Kolorowanie kawałków tortu
    series->slices().at(0)->setColor(QColor("#7f8c8d")); // Szary
    series->slices().at(1)->setColor(QColor("#2980b9")); // Niebieski
    series->slices().at(2)->setColor(QColor("#27ae60")); // Zielony
    series->slices().at(3)->setColor(QColor("#c0392b")); // Czerwony

    // Wyświetlanie etykiet tylko dla tych statusów, które mają więcej niż 0 sztuk
    for(auto slice : series->slices()) {
        if (slice->value() > 0) {
            slice->setLabelVisible(true);
            slice->setLabel(QString("%1: %2").arg(slice->label()).arg(slice->value()));
        }
    }

    // 3. Budujemy sam obiekt wykresu
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Status Biblioteki");
    chart->legend()->hide();
    chart->setAnimationOptions(QChart::SeriesAnimations);

    // 4. Przypisanie wykresu do Twojego wyklikanego pojemnika w Designerze
    // Poprzedni wykres zostaje automatycznie usunięty przez QChartView (zapobiega wyciekom pamięci)
    ui->wykresOwalny->setChart(chart);
    ui->wykresOwalny->setRenderHint(QPainter::Antialiasing); // Wygładzanie

    // --- 5. PANEL OSTATNIO EDYTOWANYCH (Siatka 2 kolumny, 3 wiersze) ---
    // Czyścimy stary grid przed nałożeniem nowych przycisków
    QLayoutItem *child;
    while ((child = ui->gridOstatnie->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    QList<int> ostatnieId = dbManager.pobierzOstatnioEdytowane(6);
    int wiersz = 0;
    int kolumna = 0;

    for (int id : ostatnieId) {
        // Szukamy obiektu w pamięci, żeby wyciągnąć tytuł
        for (const auto& m : std::as_const(listaMultimediow)) {
            if (m->getId() == id) {
                // Tworzymy duży, klikalny przycisk reprezentujący kafel
                QPushButton *btnKafel = new QPushButton(m->getTytul(), this);
                btnKafel->setMinimumHeight(50); // Żeby nie były to cienkie paski

                // Po kliknięciu używamy naszej uniwersalnej funkcji!
                connect(btnKafel, &QPushButton::clicked, this, [this, id]() {
                    pokazSzczegolyMedium(id);
                });

                ui->gridOstatnie->addWidget(btnKafel, wiersz, kolumna);

                // Matematyka dla siatki 2-kolumnowej
                kolumna++;
                if (kolumna > 1) {
                    kolumna = 0;
                    wiersz++;
                }
                break;
            }
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

void MainWindow::przygotujFormularz(int idMedium, int idDomyslnejKategorii, int idDomyslnejPlatformy) {
    uzupelnijComboBoxy();

    if (idMedium == -1) {
        // DODAWANIE
        czyTrybEdycji = false;
        ui->btnPotwierdzDodaj->setText("Dodaj do biblioteki");

        ui->editNowyTytul->clear();
        ui->spinNowyCel->setValue(1);

        // Ustawianie kategorii (jeśli podano)
        int indexKat = ui->comboNowaKategoria->findData(idDomyslnejKategorii);
        ui->comboNowaKategoria->setCurrentIndex(indexKat != -1 ? indexKat : 0);

        // Ustawianie platformy (jeśli podano)
        int indexPlat = ui->comboNowyTyp->findData(idDomyslnejPlatformy);
        ui->comboNowyTyp->setCurrentIndex(indexPlat != -1 ? indexPlat : 0);

    } else {
        // EDYCJA - bez zmian, zostaw jak było
        czyTrybEdycji = true;
        idEdytowanegoMedium = idMedium;
        ui->btnPotwierdzDodaj->setText("Zapisz zmiany");

        for (const auto& m : std::as_const(listaMultimediow)) {
            if (m->getId() == idMedium) {
                ui->editNowyTytul->setText(m->getTytul());
                ui->spinNowyCel->setValue(m->getPostep().docelowa);

                int idxKat = ui->comboNowaKategoria->findData(m->getIdKategorii());
                ui->comboNowaKategoria->setCurrentIndex(idxKat != -1 ? idxKat : 0);

                int idxPlat = ui->comboNowyTyp->findData(m->getIdPlatformy());
                ui->comboNowyTyp->setCurrentIndex(idxPlat != -1 ? idxPlat : 0);
                break;
            }
        }
    }
    ui->daneSzczegolowe->setCurrentWidget(ui->page_3);
}

void MainWindow::usunWybraneMedium(int id) {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Potwierdzenie",
                                  "Czy na pewno chcesz usunąć tę pozycję z biblioteki? \nTej operacji nie da się cofnąć.",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (dbManager.usunMedium(id)) {
            ui->statusbar->showMessage("Usunięto pomyślnie", 3000);
            zaladujDaneDoDrzewa(); // Odświeżamy listę
            ui->daneSzczegolowe->setCurrentIndex(0); // Wracamy na start
        }
    }
}

void MainWindow::uzupelnijComboBoxy() {
    ui->comboNowaKategoria->clear();
    ui->comboNowyTyp->clear();

    // 1. KATEGORIE
    ui->comboNowaKategoria->addItem("--- Wybierz kategorię ---", 0);
    // Domyślna wartość dla braku wyboru
    ui->comboNowaKategoria->setItemData(0, " . . . . ", Qt::UserRole + 1);

    auto kategorie = dbManager.pobierzKategorie();
    auto jednostki = dbManager.pobierzSlownikJednostek();

    for (const auto& kat : std::as_const(kategorie)) {
        if (kat.first != 0) {
            ui->comboNowaKategoria->addItem(kat.second, kat.first);

            // Pobieramy indeks nowo dodanego elementu (ostatniego na liście)
            int idx = ui->comboNowaKategoria->count() - 1;

            // Zapisujemy jednostkę w ukrytym schowku
            QString jedn = jednostki.value(kat.first, "Wartość");
            ui->comboNowaKategoria->setItemData(idx, jedn, Qt::UserRole + 1);
        }
    }

    // 2. PLATFORMY
    ui->comboNowyTyp->addItem("--- Wybierz platformę ---", 0);
    auto platformy = dbManager.pobierzPlatformy();
    for (const auto& plat : std::as_const(platformy)) {
        if (plat.first != 0) {
            ui->comboNowyTyp->addItem(plat.second, plat.first);
        }
    }
}

void MainWindow::pokazSzczegolyMedium(int idMedium) {
    for (const auto& medium : std::as_const(listaMultimediow)) {
        if (medium->getId() == idMedium) {

            ui->lblDetaleTytul->setText(medium->getTytul());
            ui->comboDetaleStatus->setCurrentText(medium->getStatus());

            // 2. SZUKANIE NAZWY PLATFORMY
            auto platformy = dbManager.pobierzPlatformy(); // Pobieramy listę par ID-Nazwa [cite: 20]
            QString nazwaPlat = "Nieznana platforma";

            for (const auto& p : platformy) {
                if (p.first == medium->getIdPlatformy()) {
                    nazwaPlat = p.second;
                    break;
                }
            }
            ui->labNazwaPlatformy->setText(nazwaPlat);

            if (medium->getStatus() == "Ukończone") {
                ui->lblWielkiStatus->setText("UKOŃCZONO");
                ui->lblWielkiStatus->show();
                ui->btnZacznijOdNowa->show();
                ui->spinDetaleAktualny->setDisabled(true);
                ui->spinDetaleDocelowy->setDisabled(true);
            } else {
                ui->lblWielkiStatus->hide();
                ui->btnZacznijOdNowa->hide();
                ui->frameKontrolkiPostepu->show();
                ui->spinDetaleAktualny->setDisabled(false);
                ui->spinDetaleDocelowy->setDisabled(false);
            }

            ui->spinDetaleDocelowy->setValue(medium->getPostep().docelowa);
            ui->spinDetaleAktualny->setMaximum(medium->getPostep().docelowa);
            ui->spinDetaleAktualny->setValue(medium->getPostep().aktualna);

            ui->lblDetaleTytul->setText(medium->getTytul());
            ui->comboDetaleStatus->setCurrentText(medium->getStatus());

            // Pobieramy jednostkę prosto ze struktury postępu!
            QString jednostka = medium->getPostep().jednostka;
            ui->lblDetaleJednostka->setText(jednostka);

            ui->progressBarDetale->setMaximum(medium->getPostep().docelowa);
            ui->progressBarDetale->setValue(medium->getPostep().aktualna);

            if (jednostka == "seanse" || medium->getPostep().docelowa == 1) {
                ui->progressBarDetale->hide();
                ui->frameKontrolkiPostepu->hide();
            } else {
                ui->progressBarDetale->show();
                ui->frameKontrolkiPostepu->show();
            }

            ui->daneSzczegolowe->setCurrentWidget(ui->page_2);
            break;
        }
    }
}