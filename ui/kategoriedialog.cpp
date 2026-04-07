#include "kategoriedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlQuery>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>

KategorieDialog::KategorieDialog(DatabaseManager& db, QWidget *parent)
    : QDialog(parent), dbManager(db)
{
    setWindowTitle("Zarządzanie Kategoriami");
    resize(500, 400);

    QVBoxLayout *layout = new QVBoxLayout(this);

    tabela = new QTableWidget(0, 3, this);
    tabela->setHorizontalHeaderLabels({"ID", "Nazwa", "Jednostka"});
    tabela->setSelectionBehavior(QAbstractItemView::SelectRows);
    tabela->setSelectionMode(QAbstractItemView::SingleSelection);
    tabela->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tabela->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    tabela->hideColumn(0);

    wypelnijTabele();
    layout->addWidget(tabela);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnDodaj = new QPushButton("Dodaj nową", this);
    btnEdytuj = new QPushButton("Edytuj wybraną", this);
    btnUsun = new QPushButton("Usuń wybraną", this);
    btnZamknij = new QPushButton("Zamknij", this);

    btnLayout->addWidget(btnDodaj);
    btnLayout->addWidget(btnEdytuj);
    btnLayout->addWidget(btnUsun);
    btnLayout->addStretch();
    btnLayout->addWidget(btnZamknij);
    layout->addLayout(btnLayout);

    connect(btnZamknij, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnDodaj, &QPushButton::clicked, this, &KategorieDialog::onBtnDodajClicked);
    connect(btnEdytuj, &QPushButton::clicked, this, &KategorieDialog::onBtnEdytujClicked);
    connect(btnUsun, &QPushButton::clicked, this, &KategorieDialog::onBtnUsunClicked);
}

KategorieDialog::~KategorieDialog() {}

void KategorieDialog::wypelnijTabele() {
    tabela->setRowCount(0);
    // Skoro DatabaseManager trzyma połączenie jako domyślne dla aplikacji, to QSqlQuery zadziała idealnie
    QSqlQuery q("SELECT id, nazwa, jednostka FROM kategorie ORDER BY nazwa");
    int wiersz = 0;
    while(q.next()) {
        tabela->insertRow(wiersz);
        tabela->setItem(wiersz, 0, new QTableWidgetItem(q.value(0).toString()));
        tabela->setItem(wiersz, 1, new QTableWidgetItem(q.value(1).toString()));
        tabela->setItem(wiersz, 2, new QTableWidgetItem(q.value(2).toString()));
        wiersz++;
    }
}

void KategorieDialog::onBtnDodajClicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("Nowa Kategoria");
    dialog.resize(300, 100);

    QFormLayout form(&dialog);
    QLineEdit editNazwa(&dialog);
    editNazwa.setPlaceholderText("np. Komiksy");

    QComboBox comboJednostka(&dialog);
    comboJednostka.setEditable(true);
    comboJednostka.addItems(dbManager.pobierzUnikalneJednostki());

    form.addRow("Nazwa kategorii:", &editNazwa);
    form.addRow("Jednostka:", &comboJednostka);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString nazwa = editNazwa.text().trimmed();
        QString jednostka = comboJednostka.currentText().trimmed();

        if (!nazwa.isEmpty()) {
            if (jednostka.isEmpty()) jednostka = "szt.";
            if (dbManager.dodajKategorie(nazwa, jednostka) > 0) {
                wypelnijTabele(); // Odświeżamy tabelę w naszym nowym oknie
            } else {
                QMessageBox::warning(this, "Błąd", "Baza odrzuciła kategorię.");
            }
        }
    }
}

void KategorieDialog::onBtnEdytujClicked() {
    int wiersz = tabela->currentRow();
    if (wiersz < 0) {
        QMessageBox::warning(this, "Brak wyboru", "Zaznacz najpierw kategorię na liście.");
        return;
    }

    int idKat = tabela->item(wiersz, 0)->text().toInt();
    QString obecnaNazwa = tabela->item(wiersz, 1)->text();
    QString obecnaJednostka = tabela->item(wiersz, 2)->text();

    QDialog editDialog(this);
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
}

void KategorieDialog::onBtnUsunClicked() {
    int wiersz = tabela->currentRow();
    if (wiersz < 0) return;

    int idKat = tabela->item(wiersz, 0)->text().toInt();
    QString nazwa = tabela->item(wiersz, 1)->text();

    QMessageBox msgBox(this);
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
}