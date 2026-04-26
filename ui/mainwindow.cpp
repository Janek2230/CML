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
    ui->comboGrupowanie->addItems({"Kategoria", "Status", "Platforma", "Data dodania"});

    ui->kategorie->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->kategorie->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // --- PROSTE LAMBDY (Zostawiamy, bo to tylko 1-2 linijki logiki UI) ---
    connect(ui->comboGrupowanie, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        zaladujDaneDoDrzewa();
    });

    connect(ui->btnPowrot, &QPushButton::clicked, this, [this]() {
        ui->daneSzczegolowe->setCurrentWidget(dashboardWidget);
        dashboardWidget->odswiezStatystykiGlowne();
    });


    // --- STANDARDOWE POŁĄCZENIA SLOTÓW ---
    connect(ui->kategorie, &QTreeWidget::itemClicked, this, &MainWindow::onWybieranieElementuDrzewa);
    connect(ui->wyszukiwarka, &QLineEdit::textChanged, this, &MainWindow::onWyszukiwanie);
    connect(ui->kategorie, &QTreeWidget::customContextMenuRequested, this, &MainWindow::pokazMenuDrzewa);

    connect(ui->btnLosuj, &QPushButton::clicked, this, &MainWindow::onBtnLosujClicked);

    // --- AKCJE Z MENU GŁÓWNEGO ---
    connect(ui->actionKategorie, &QAction::triggered, this, [this]() {
        KategorieDialog dialog(dbManager, this);
        dialog.exec();
        listaMultimediow = dbManager.getAllMultimedia();
        zaladujDaneDoDrzewa();
    });

    connect(ui->actionPlatformy, &QAction::triggered, this, [this]() {
        PlatformyDialog dialog(dbManager, this);
        dialog.exec();
        listaMultimediow = dbManager.getAllMultimedia();
        zaladujDaneDoDrzewa();
    });

    connect(ui->actionStronaGlowna, &QAction::triggered, this, [this]() {
            ui->panelKategorii->show();
            ui->daneSzczegolowe->setCurrentWidget(dashboardWidget);
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
        dashboardWidget->odswiezStatystykiGlowne();
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
        dashboardWidget->odswiezStatystykiGlowne();
        // Wracamy do widoku domyślnego
        ui->daneSzczegolowe->setCurrentWidget(dashboardWidget);
    });

    // Kiedy formularz krzyknie, że go anulowano:
    connect(formularzWidget, &MultimediaFormWidget::formularzAnulowany, this, [this]() {
        ui->daneSzczegolowe->setCurrentWidget(dashboardWidget);
    });

    connect(ui->btnDodajMedium, &QPushButton::clicked, this, [this]() {
        formularzWidget->przygotujFormularz(-1, 0, 0);
        ui->daneSzczegolowe->setCurrentWidget(formularzWidget);
    });

    szczegolyWidget = new SzczegolyWidget(dbManager, this);
    ui->daneSzczegolowe->addWidget(szczegolyWidget); // Wrzucamy na stos widoków

    connect(szczegolyWidget, &SzczegolyWidget::daneZaktualizowane, this, [this]() {
        listaMultimediow = dbManager.getAllMultimedia(); // Przeładowanie po zmianie
        zaladujDaneDoDrzewa();
        dashboardWidget->odswiezStatystykiGlowne();
    });

    dashboardWidget = new DashboardWidget(dbManager, this);
    ui->daneSzczegolowe->addWidget(dashboardWidget);

    // Jeśli dashboard krzyczy, że trzeba pokazać szczegóły (bo kliknięto losowanie lub kafel), MainWindow to robi
    connect(dashboardWidget, &DashboardWidget::zadaniePokazaniaSzczegolow, this, [this](int id) {
        pokazSzczegolyMedium(id);
        ui->statusbar->showMessage("Przełączono na szczegóły!", 3000); // Mały bonus dla wylosowanego
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

void MainWindow::onWybieranieElementuDrzewa(QTreeWidgetItem *item, int column) {
    if (!item || item->parent() == nullptr) {
        if (item) item->setExpanded(!item->isExpanded());
        if (ui->daneSzczegolowe->currentWidget() != ui->page) {
            dashboardWidget->odswiezStatystykiGlowne();
            ui->daneSzczegolowe->setCurrentWidget(dashboardWidget);
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
    szczegolyWidget->ustawMedium(idMedium);
    ui->daneSzczegolowe->setCurrentWidget(szczegolyWidget);
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
                        dashboardWidget->odswiezStatystykiGlowne();
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
                formularzWidget->przygotujFormularz(-1, idKategorii, 0);
                ui->daneSzczegolowe->setCurrentWidget(formularzWidget);
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
                        dashboardWidget->odswiezStatystykiGlowne();
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
                formularzWidget->przygotujFormularz(-1, 0, idPlatformy);
                ui->daneSzczegolowe->setCurrentWidget(formularzWidget);
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
                        dashboardWidget->odswiezStatystykiGlowne();
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
            formularzWidget->przygotujFormularz(idMedium, 0); // Tryb edycji na podstawie ID!
            ui->daneSzczegolowe->setCurrentWidget(formularzWidget);
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
                    dashboardWidget->odswiezStatystykiGlowne();
                    ui->daneSzczegolowe->setCurrentIndex(0);
                }
            }
        });
    }

    // Wyświetlamy menu w miejscu kursora myszy
    menu.exec(ui->kategorie->mapToGlobal(pos));
}

