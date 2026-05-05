#include "tagidialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>

TagiDialog::TagiDialog(AppController& controller, QWidget *parent)
    : QDialog(parent), appController(controller)
{
    setWindowTitle("Zarządzanie Tagami");
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
    btnDodaj = new QPushButton("Dodaj nowy", this);
    btnEdytuj = new QPushButton("Edytuj wybrany", this);
    btnUsun = new QPushButton("Usuń wybrany", this);
    btnZamknij = new QPushButton("Zamknij", this);

    btnLayout->addWidget(btnDodaj);
    btnLayout->addWidget(btnEdytuj);
    btnLayout->addWidget(btnUsun);
    btnLayout->addStretch();
    btnLayout->addWidget(btnZamknij);
    layout->addLayout(btnLayout);

    connect(btnZamknij, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnDodaj, &QPushButton::clicked, this, &TagiDialog::onBtnDodajClicked);
    connect(btnEdytuj, &QPushButton::clicked, this, &TagiDialog::onBtnEdytujClicked);
    connect(btnUsun, &QPushButton::clicked, this, &TagiDialog::onBtnUsunClicked);
}

TagiDialog::~TagiDialog() {}

void TagiDialog::wypelnijTabele() {
    tabela->setRowCount(0);
    auto tagi = appController.pobierzTagi();
    int wiersz = 0;
    for (const auto& tag : tagi) {
        tabela->insertRow(wiersz);
        tabela->setItem(wiersz, 0, new QTableWidgetItem(QString::number(tag.first)));
        tabela->setItem(wiersz, 1, new QTableWidgetItem(tag.second));
        wiersz++;
    }
}

void TagiDialog::onBtnDodajClicked() {
    bool ok = false;
    QString nazwa = QInputDialog::getText(this, "Nowy tag", "Nazwa:", QLineEdit::Normal, "", &ok).trimmed();
    if (!ok || nazwa.isEmpty()) {
        return;
    }

    if (appController.dodajTag(nazwa) > 0) {
        wypelnijTabele();
    } else {
        QMessageBox::warning(this, "Błąd", "Nie udało się dodać tagu (może już istnieje).");
    }
}

void TagiDialog::onBtnEdytujClicked() {
    int wiersz = tabela->currentRow();
    if (wiersz < 0) {
        QMessageBox::warning(this, "Brak wyboru", "Zaznacz tag na liście.");
        return;
    }

    int idTagu = tabela->item(wiersz, 0)->text().toInt();
    QString obecnaNazwa = tabela->item(wiersz, 1)->text();

    bool ok = false;
    QString nowaNazwa = QInputDialog::getText(this, "Edytuj tag", "Nazwa:", QLineEdit::Normal, obecnaNazwa, &ok).trimmed();
    if (!ok || nowaNazwa.isEmpty()) {
        return;
    }

    if (appController.aktualizujTag(idTagu, nowaNazwa)) {
        wypelnijTabele();
    } else {
        QMessageBox::warning(this, "Błąd", "Nie udało się zaktualizować tagu.");
    }
}

void TagiDialog::onBtnUsunClicked() {
    int wiersz = tabela->currentRow();
    if (wiersz < 0) {
        return;
    }

    int idTagu = tabela->item(wiersz, 0)->text().toInt();
    QString nazwa = tabela->item(wiersz, 1)->text();

    auto odp = QMessageBox::question(
        this,
        "Usuń tag",
        QString("Czy na pewno usunąć tag \"%1\"?\nPowiązania z mediami zostaną usunięte.").arg(nazwa),
        QMessageBox::Yes | QMessageBox::No
    );
    if (odp != QMessageBox::Yes) {
        return;
    }

    if (appController.usunTag(idTagu)) {
        wypelnijTabele();
    } else {
        QMessageBox::warning(this, "Błąd", "Nie udało się usunąć tagu.");
    }
}
