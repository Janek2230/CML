#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qlineedit.h>
#include <utility>
#include <QDebug>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->comboDetaleStatus->addItem("Planowane");
    ui->comboDetaleStatus->addItem("W trakcie");
    ui->comboDetaleStatus->addItem("Ukończone");
    ui->comboDetaleStatus->addItem("Porzucone");

    ui->kategorie->setContextMenuPolicy(Qt::CustomContextMenu);

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
        QString tytul = ui->editNowyTytul->text();
        if (tytul.isEmpty()) {
            QMessageBox::warning(this, "Błąd", "Tytuł nie może być pusty!");
            return;
        }

        int idKat = ui->comboNowaKategoria->currentData().toInt();
        QString jednostka = ui->comboNowyTyp->currentText();
        int cel = ui->spinNowyCel->value();

        if (dbManager.dodajNoweMedium(tytul, idKat, jednostka, cel)) {
            ui->statusbar->showMessage("Dodano: " + tytul, 3000);

            // KLUCZOWE: Odświeżamy wszystko
            listaMultimediow = dbManager.getAllMultimedia(); // Pobieramy nową listę z bazy
            zaladujDaneDoDrzewa(); // Przebudowujemy drzewko po lewej
            odswiezStatystykiGlowne(); // Aktualizujemy licznik na startowej
            uzupelnijComboBoxy();

            ui->daneSzczegolowe->setCurrentWidget(ui->page); // Wracamy na stronę główną
        }
    });

    connect(ui->btnDodajMedium, &QPushButton::clicked, this, [this]() {
        // 0 to ID dla "Brak kategorii" w naszej bazie
        otworzFormularzDodawania(0);
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
        // Kliknięto w nagłówek (Gry, Książki), a nie w konkretny tytuł
        if (item) item->setExpanded(!item->isExpanded());
        return;
    }



    // Wyciągamy ukryte dane ze schowków (ID oraz jednostkę, które zapisaliśmy przy budowaniu drzewa)
    int idWybranegoElementu = item->data(0, Qt::UserRole).toInt();
    QString jednostka = item->data(0, Qt::UserRole + 1).toString();

    // Szukamy tego elementu na naszej liście pobranej z bazy
    for (const auto& medium : std::as_const(listaMultimediow)) {


        if (medium->getStatus() == "Ukończone") {
            // Pokazujemy napis i przycisk
            ui->lblWielkiStatus->setText("UKOŃCZONO");
            ui->lblWielkiStatus->show();
            ui->btnZacznijOdNowa->show(); // <--- Opcja aktywna!

            ui->spinDetaleAktualny->setDisabled(true);
            ui->spinDetaleDocelowy->setDisabled(true);
        } else {
            // Ukrywamy, jeśli gra/książka jest w trakcie lub planowana
            ui->lblWielkiStatus->hide();
            ui->btnZacznijOdNowa->hide(); // <--- Opcja ukryta!

            ui->frameKontrolkiPostepu->show();
            ui->spinDetaleAktualny->setDisabled(false);
            ui->spinDetaleDocelowy->setDisabled(false);

        }

        if (medium->getId() == idWybranegoElementu) {
            ui->spinDetaleDocelowy->setValue(medium->getPostep().docelowa);
            ui->spinDetaleAktualny->setMaximum(medium->getPostep().docelowa); // Ustawiamy limit przed wpisaniem wartości!
            ui->spinDetaleAktualny->setValue(medium->getPostep().aktualna);

            // 1. Wypełniamy Tytuł
            ui->lblDetaleTytul->setText(medium->getTytul());

            // 2. Wypełniamy Combo ze statusem (zakładam, że dodałeś tam już opcje w Designerze)
            ui->comboDetaleStatus->setCurrentText(medium->getStatus());

            // 3. Wypełniamy wartości liczbowe postępu
            ui->spinDetaleAktualny->setValue(medium->getPostep().aktualna);
            ui->spinDetaleDocelowy->setValue(medium->getPostep().docelowa);
            ui->lblDetaleJednostka->setText(jednostka);

            // 4. Ustawiamy Pasek Postępu
            ui->progressBarDetale->setMaximum(medium->getPostep().docelowa);
            ui->progressBarDetale->setValue(medium->getPostep().aktualna);

            // Ukrywamy pasek postępu, jeśli to film (target = 1)
            if (jednostka == "seans" || medium->getPostep().docelowa == 1) {
                ui->progressBarDetale->hide();
                ui->frameKontrolkiPostepu->hide();
            } else {
                ui->progressBarDetale->show();
                ui->frameKontrolkiPostepu->show();
            }

            // 5. Przełączamy QStackedWidget na stronę z detalami! (Index 1 to zazwyczaj page_2)
            ui->daneSzczegolowe->setCurrentWidget(ui->page_2);
            break; // Znaleźliśmy, więc kończymy pętlę
        }
    }
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

    // 1. Najpierw ładujemy słownik kategorii z bazy
    QMap<int, QString> kategorie = dbManager.getCategories();

    // Mapa ułatwiająca nam znalezienie wskaźnika na główną gałąź na podstawie jej ID z bazy
    QMap<int, QTreeWidgetItem*> wezlyKategorii;

    // 2. Dynamicznie budujemy nagłówki (Top-Level Items)
    for (auto it = kategorie.begin(); it != kategorie.end(); ++it) {
        QTreeWidgetItem *katItem = new QTreeWidgetItem(ui->kategorie);
        katItem->setText(0, it.value()); // np. "Gry", "Książki"
        wezlyKategorii.insert(it.key(), katItem); // Zapisujemy, żeby mieć gdzie przypinać dzieci
    }

    // 3. Pobieramy konkretne pozycje
    listaMultimediow = dbManager.getAllMultimedia();

    // 4. Przypinamy pozycje do ich odpowiednich rodziców
    for (const auto& medium : std::as_const(listaMultimediow)) {
        int idKat = medium->getIdKategorii();

        // Upewniamy się, że kategoria dla tego medium w ogóle istnieje na drzewie
        if (wezlyKategorii.contains(idKat)) {
            // Tworzymy dziecko pod odpowiednim rodzicem
            QTreeWidgetItem *item = new QTreeWidgetItem(wezlyKategorii[idKat]);
            item->setText(0, medium->getTytul());

            // Zapisujemy krytyczne dane w ukrytych schowkach Qt
            item->setData(0, Qt::UserRole, medium->getId());
            item->setData(0, Qt::UserRole + 1, medium->getPostep().jednostka);
        }
    }

    // 5. Rozwijamy drzewo, żeby nie wyglądało na puste po starcie
    ui->kategorie->expandAll();
}

void MainWindow::odswiezStatystykiGlowne() {
    auto stats = dbManager.getGlobalStats();

    // Ustawiamy wartości w UI (pamiętaj o konwersji int na QString)
    ui->lblStatWszystkie->setText(QString::number(stats.value("Suma", 0)));
    ui->lblStatWTrakcie->setText(QString::number(stats.value("W trakcie", 0)));
    ui->lblStatUkonczone->setText(QString::number(stats.value("Ukończone", 0)));

    // Upewniamy się, że po starcie widzimy właśnie tę stronę
    //ui->daneSzczegolowe->setCurrentIndex(0);
}

void MainWindow::pokazMenuDrzewa(const QPoint &pos) {
    // Sprawdzamy, na co dokładnie kliknął użytkownik
    QTreeWidgetItem *kliknietyElement = ui->kategorie->itemAt(pos);

    QMenu menu(this); // Tworzymy puste menu

    if (!kliknietyElement) {
        // 1. KLIKNIĘTO W PUSTE MIEJSCE (Pod listą)
        menu.addAction("Dodaj nową kategorię", this, []() {
            // Tu wywołamy funkcję dodawania kategorii
            qDebug() << "Dodaję kategorię...";
        });
    }
    else if (kliknietyElement->parent() == nullptr) {
        // 2. KLIKNIĘTO W KATEGORIĘ GŁÓWNĄ (np. "Gry")
        menu.addAction("Dodaj pozycję do tej kategorii", this, [kliknietyElement]() {
            qDebug() << "Dodaję do: " << kliknietyElement->text(0);
        });

        // Zabezpieczenie przed edycją/usunięciem "Brak kategorii"
        if (kliknietyElement->text(0) != "Brak kategorii") {
            menu.addAction("Zmień nazwę kategorii", this, [kliknietyElement]() {
                qDebug() << "Zmieniam nazwę: " << kliknietyElement->text(0);
            });
            menu.addSeparator(); // Fajna kreseczka oddzielająca akcje destrukcyjne
            menu.addAction("Usuń kategorię", this, [kliknietyElement]() {
                qDebug() << "Usuwam: " << kliknietyElement->text(0);
            });
        }
    }
    else {
        // 3. KLIKNIĘTO W KONKRETNE MEDIUM (np. "Cyberpunk 2077")
        menu.addAction("Edytuj pozycję", this, [kliknietyElement]() {
            qDebug() << "Edytuję: " << kliknietyElement->text(0);
        });
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

void MainWindow::otworzFormularzDodawania(int domyslneIdKategorii) {
    // 1. Czyścimy pola po poprzednim użyciu
    ui->editNowyTytul->clear();
    ui->spinNowyCel->setValue(1);

    // 2. Ustawiamy kategorię w ComboBoksie na podstawie ID
    // Zakładamy, że w comboNowaKategoria przechowujemy ID kategorii w UserRole
    int index = ui->comboNowaKategoria->findData(domyslneIdKategorii);
    if (index != -1) {
        ui->comboNowaKategoria->setCurrentIndex(index);
    } else {
        // Jeśli nie znaleziono (np. błąd), ustawiamy na "Brak kategorii" (ID 0)
        ui->comboNowaKategoria->setCurrentIndex(ui->comboNowaKategoria->findData(0));
    }

    // 3. Przełączamy widok
    ui->daneSzczegolowe->setCurrentWidget(ui->page_3); // page_3
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
    // 1. Czyścimy stare śmieci
    ui->comboNowaKategoria->clear();
    ui->comboNowyTyp->clear();

    // 2. Ładujemy kategorie z bazy
    auto kategorie = dbManager.pobierzKategorie();
    for (const auto& kat : std::as_const(kategorie)) {
        // addItem przyjmuje: (Tekst_Dla_Użytkownika, Ukryte_Dane)
        // kat.second to nazwa, kat.first to ID z bazy!
        ui->comboNowaKategoria->addItem(kat.second, kat.first);
    }

    // 3. Ładujemy platformy/jednostki
    QStringList platformy = dbManager.pobierzPlatformy();
    ui->comboNowyTyp->addItems(platformy);
}
