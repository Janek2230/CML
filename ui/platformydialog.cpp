#include "platformydialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlQuery>
#include <QInputDialog>

PlatformyDialog::PlatformyDialog(AppController& controller, QWidget *parent)
    : QDialog(parent), appController(controller)
{
    setWindowTitle("Zarządzanie Platformami");
    resize(400, 400);

    QVBoxLayout *layout = new QVBoxLayout(this);

    tabela = new QTableWidget(0, 2, this);
    tabela->setHorizontalHeaderLabels({"ID", "Nazwa"});
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
    connect(btnDodaj, &QPushButton::clicked, this, &PlatformyDialog::onBtnDodajClicked);
    connect(btnEdytuj, &QPushButton::clicked, this, &PlatformyDialog::onBtnEdytujClicked);
    connect(btnUsun, &QPushButton::clicked, this, &PlatformyDialog::onBtnUsunClicked);
}

PlatformyDialog::~PlatformyDialog() {}

void PlatformyDialog::wypelnijTabele() {
    tabela->setRowCount(0);
    QSqlQuery q("SELECT id, nazwa FROM platformy ORDER BY nazwa");
    int wiersz = 0;
    while(q.next()) {
        tabela->insertRow(wiersz);
        tabela->setItem(wiersz, 0, new QTableWidgetItem(q.value(0).toString()));
        tabela->setItem(wiersz, 1, new QTableWidgetItem(q.value(1).toString()));
        wiersz++;
    }
}

void PlatformyDialog::onBtnDodajClicked() {
    bool ok;
    QString nowaNazwa = QInputDialog::getText(this, "Nowa Platforma", "Nazwa:", QLineEdit::Normal, "", &ok);
    if (ok && !nowaNazwa.trimmed().isEmpty()) {
        if (appController.dodajPlatforme(nowaNazwa.trimmed()) > 0) {
            wypelnijTabele();
        } else {
            QMessageBox::warning(this, "Błąd", "Nie udało się dodać platformy.");
        }
    }
}

void PlatformyDialog::onBtnEdytujClicked() {
    int wiersz = tabela->currentRow();
    if (wiersz < 0) {
        QMessageBox::warning(this, "Brak wyboru", "Zaznacz platformę na liście.");
        return;
    }

    int idPlat = tabela->item(wiersz, 0)->text().toInt();
    QString obecnaNazwa = tabela->item(wiersz, 1)->text();

    bool ok;
    QString nowaNazwa = QInputDialog::getText(this, "Edytuj Platformę", "Nazwa:", QLineEdit::Normal, obecnaNazwa, &ok);

    if (ok && !nowaNazwa.trimmed().isEmpty()) {
        if (appController.aktualizujPlatforme(idPlat, nowaNazwa.trimmed())) {
            wypelnijTabele();
        }
    }
}

void PlatformyDialog::onBtnUsunClicked() {
    int wiersz = tabela->currentRow();
    if (wiersz < 0) return;

    int idPlat = tabela->item(wiersz, 0)->text().toInt();
    QString nazwa = tabela->item(wiersz, 1)->text();

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Usuwanie");
    msgBox.setText("Usuwasz platformę: " + nazwa + "\nCo robimy z zawartością?");

    msgBox.addButton("Przenieś do 'Brak'", QMessageBox::ActionRole);
    QPushButton *btnUsunWszystko = msgBox.addButton("Usuń wszystko", QMessageBox::DestructiveRole);
    QPushButton *btnAnuluj = msgBox.addButton("Anuluj", QMessageBox::RejectRole);

    msgBox.exec();

    if (msgBox.clickedButton() == btnAnuluj) return;

    bool usunZDetalami = (msgBox.clickedButton() == btnUsunWszystko);
    if (appController.usunPlatforme(idPlat, usunZDetalami)) {
        wypelnijTabele();
    }
}
