#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qlineedit.h>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->kategorie, &QTreeWidget::itemClicked,
            this, &MainWindow::onWybieranieElementuDrzewa);

    connect(ui->wyszukiwarka, &QLineEdit::textChanged,
            this, &MainWindow::onWyszukiwanieZmienione);

    if (dbManager.openConnection()) {
        zaladujDaneDoDrzewa();
    } else {
        qDebug() << "Baza leży i kwiczy, nie ładuję drzewa.";
        // W przyszłości wrzucisz tu wyskakujące okienko (QMessageBox) dla użytkownika
    }
}

MainWindow::~MainWindow()
{
    listaKsiazek.clear();

    delete ui;
}

void MainWindow::onWybieranieElementuDrzewa(QTreeWidgetItem *item, int column) {
    // Zabezpieczenie przed głupotą - upewniamy się, że element istnieje
    if (!item) return;

    // Sprawdzamy, czy to kategoria główna (Top-Level Item)
    if (item->parent() == nullptr) {
        // Kliknięto nagłówek (np. "Gry", "Książki").
        // Zamiast ładować dane, po prostu zwijamy lub rozwijamy listę.
        item->setExpanded(!item->isExpanded());
        return; // Przerywamy działanie funkcji, tu nie ma czego wyświetlać w szczegółach
    }

    // Skoro element ma rodzica, to jest to konkretny tytuł multimediów!

    // W tym miejscu musisz wyciągnąć informacje o tym, W CO dokładnie kliknięto.
    // Najlepiej, jeśli podczas budowania drzewa ukryłeś ID z bazy danych
    // wewnątrz tego QTreeWidgetItem (zaraz o to zapytam).
    int idWybranegoElementu = item->data(0, Qt::UserRole).toInt();

    // Tutaj normalnie wywołałbyś coś w stylu:
    // auto szczegoly = dbManager.pobierzSzczegoly(idWybranegoElementu);
    // ui->labelTytul->setText(szczegoly->getTytul());
    // ui->progressBar->setValue(szczegoly->getProcentPostepu());

    // A na koniec, wisienka na torcie: Przełączasz stronę w prawym panelu
    // Ustawia stronę na indeksie 0 (czyli pierwszą stronę, którą zaprojektowałeś)
    ui->daneSzczegolowe->setCurrentIndex(0);
}

void MainWindow::onWyszukiwanieZmienione(const QString &text) {
    // Na razie pusto.
    // Tu za chwilę napiszemy logikę filtrowania drzewa,
    // ale najpierw program musi się w ogóle skompilować.
}

void MainWindow::zaladujDaneDoDrzewa() {
    // 1. Zawsze czyść drzewo na początku, żeby nie duplikować danych przy odświeżaniu
    ui->kategorie->clear();

    // 2. Tworzymy Główne Kategorie (Top-Level Items)
    QTreeWidgetItem *katKsiazki = new QTreeWidgetItem(ui->kategorie);
    katKsiazki->setText(0, "Książki");

    QTreeWidgetItem *katGry = new QTreeWidgetItem(ui->kategorie);
    katGry->setText(0, "Gry");

    QTreeWidgetItem *katFilmy = new QTreeWidgetItem(ui->kategorie);
    katFilmy->setText(0, "Filmy");

    // 3. Pobieramy dane z bazy (Na razie masz tylko książki, więc skupmy się na nich)
    listaKsiazek = dbManager.getBooks();

    // 4. Przypinamy książki do kategorii "Książki"
    for (const auto& ksiazka : listaKsiazek) {
        // Tworzymy nowy element jako "dziecko" kategorii Książki
        QTreeWidgetItem *item = new QTreeWidgetItem(katKsiazki);

        // Ustawiamy widoczny tekst
        item->setText(0, ksiazka->getTytul());

        // MAGIA: Zapisujemy ID z bazy danych w sposób niewidoczny dla użytkownika!
        // Qt::UserRole to specjalny "schowek" w każdym elemencie UI do przechowywania dowolnych danych.
        item->setData(0, Qt::UserRole, ksiazka->getId());
    }

    // 5. Opcjonalnie: Rozwińmy kategorię książek domyślnie, żeby nie było pusto
    katKsiazki->setExpanded(true);
}