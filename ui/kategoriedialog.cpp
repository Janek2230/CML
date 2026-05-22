#include "kategoriedialog.h"
#include "ui_kategoriedialog.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>

KategorieDialog::KategorieDialog(AppController& controller, QWidget *parent)
    : QDialog(parent), ui(new Ui::KategorieDialog), appController(controller)
{
    ui->setupUi(this);

    // Kolumna ID (indeks 0) jest potrzebna w kodzie do identyfikacji wiersza,
    // ale nie ma wartości informacyjnej dla użytkownika.
    ui->tabela->hideColumn(0);

    // Kolumna "Nazwa" rozciąga się na całą dostępną szerokość.
    ui->tabela->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    wypelnijTabele();

    connect(ui->btnDodaj,  &QPushButton::clicked, this, &KategorieDialog::onBtnDodajClicked);
    connect(ui->btnEdytuj, &QPushButton::clicked, this, &KategorieDialog::onBtnEdytujClicked);
    connect(ui->btnUsun,   &QPushButton::clicked, this, &KategorieDialog::onBtnUsunClicked);
    // btnZamknij accept() jest podłączony w pliku .ui
}

KategorieDialog::~KategorieDialog()
{
    delete ui;
}

void KategorieDialog::wypelnijTabele()
{
    ui->tabela->setRowCount(0);
    const auto kategorie = appController.pobierzPelneKategorie();

    int wiersz = 0;
    for (const auto& kat : kategorie) {
        ui->tabela->insertRow(wiersz);
        ui->tabela->setItem(wiersz, 0, new QTableWidgetItem(QString::number(kat.id)));
        ui->tabela->setItem(wiersz, 1, new QTableWidgetItem(kat.nazwa));
        ui->tabela->setItem(wiersz, 2, new QTableWidgetItem(kat.jednostka));
        wiersz++;
    }
}

void KategorieDialog::onBtnDodajClicked()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Nowa Kategoria");
    dialog.resize(300, 120);

    QFormLayout form(&dialog);

    QLineEdit editNazwa(&dialog);
    editNazwa.setPlaceholderText("np. Komiksy");

    // Combo jest edytowalne — użytkownik może wpisać własną jednostkę
    // zamiast wybierać z listy istniejących.
    QComboBox comboJednostka(&dialog);
    comboJednostka.setEditable(true);
    comboJednostka.addItems(appController.pobierzUnikalneJednostki());

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);

    form.addRow("Nazwa kategorii:", &editNazwa);
    form.addRow("Jednostka:",       &comboJednostka);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) return;

    const QString nazwa     = editNazwa.text().trimmed();
    const QString jednostka = comboJednostka.currentText().trimmed();

    if (nazwa.isEmpty()) return;

    // Fallback na "szt." gdy użytkownik zostawił pole jednostki puste.
    if (!appController.dodajKategorie(nazwa, jednostka.isEmpty() ? "szt." : jednostka)) {
        QMessageBox::warning(this, "Błąd", "Nie udało się dodać kategorii.");
        return;
    }

    wypelnijTabele();
}

void KategorieDialog::onBtnEdytujClicked()
{
    const int wiersz = ui->tabela->currentRow();
    if (wiersz < 0) {
        QMessageBox::warning(this, "Brak wyboru", "Zaznacz najpierw kategorię na liście.");
        return;
    }

    const int     idKat           = ui->tabela->item(wiersz, 0)->text().toInt();
    const QString obecnaNazwa     = ui->tabela->item(wiersz, 1)->text();
    const QString obecnaJednostka = ui->tabela->item(wiersz, 2)->text();

    QDialog editDialog(this);
    editDialog.setWindowTitle("Edytuj: " + obecnaNazwa);

    QFormLayout form(&editDialog);

    QLineEdit editNazwa(&editDialog);
    editNazwa.setText(obecnaNazwa);

    QComboBox comboJednostka(&editDialog);
    comboJednostka.setEditable(true);
    comboJednostka.addItems(appController.pobierzUnikalneJednostki());
    comboJednostka.setCurrentText(obecnaJednostka);

    QDialogButtonBox bb(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &editDialog);

    form.addRow("Nazwa:",     &editNazwa);
    form.addRow("Jednostka:", &comboJednostka);
    form.addRow(&bb);

    connect(&bb, &QDialogButtonBox::accepted, &editDialog, &QDialog::accept);
    connect(&bb, &QDialogButtonBox::rejected, &editDialog, &QDialog::reject);

    if (editDialog.exec() != QDialog::Accepted) return;

    const QString nowaNazwa = editNazwa.text().trimmed();
    if (nowaNazwa.isEmpty()) return;

    if (appController.aktualizujKategorie(idKat, nowaNazwa, comboJednostka.currentText().trimmed())) {
        wypelnijTabele();
    }
}

void KategorieDialog::onBtnUsunClicked()
{
    const int wiersz = ui->tabela->currentRow();
    if (wiersz < 0) return;

    const int     idKat = ui->tabela->item(wiersz, 0)->text().toInt();
    const QString nazwa = ui->tabela->item(wiersz, 1)->text();

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Usuwanie kategorii");
    msgBox.setText("Usuwasz kategorię: <b>" + nazwa + "</b><br>Co robimy z przypisaną zawartością?");

    // Musimy zachować wskaźniki do przycisków, żeby później sprawdzić który
    // został kliknięty przez msgBox.clickedButton().
    QPushButton *btnPrzenies     = msgBox.addButton("Przenieś do 'Brak'", QMessageBox::ActionRole);
    QPushButton *btnUsunWszystko = msgBox.addButton("Usuń wszystko",      QMessageBox::DestructiveRole);
    msgBox.addButton("Anuluj", QMessageBox::RejectRole);

    msgBox.exec();

    QAbstractButton *clicked = msgBox.clickedButton();
    if (clicked != btnPrzenies && clicked != btnUsunWszystko) return;

    const bool usunZDetalami = (clicked == btnUsunWszystko);
    if (appController.usunKategorie(idKat, usunZDetalami)) {
        wypelnijTabele();
    }
}
