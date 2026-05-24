#include "tagidialog.h"
#include "ui_tagidialog.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>

TagiDialog::TagiDialog(AppController& controller, QWidget *parent)
    : QDialog(parent), ui(new Ui::TagiDialog), appController(controller)
{
    ui->setupUi(this);

    ui->tabela->hideColumn(0);
    ui->tabela->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    wypelnijTabele();

    connect(ui->btnDodaj,    &QPushButton::clicked, this, &TagiDialog::onBtnDodajClicked);
    connect(ui->btnEdytuj,   &QPushButton::clicked, this, &TagiDialog::onBtnEdytujClicked);
    connect(ui->btnUsun,     &QPushButton::clicked, this, &TagiDialog::onBtnUsunClicked);
    connect(ui->btnZamknij,  &QPushButton::clicked, this, &QDialog::accept);
}

TagiDialog::~TagiDialog()
{
    delete ui;
}

void TagiDialog::wypelnijTabele()
{
    ui->tabela->setRowCount(0);
    const auto tagi = appController.pobierzTagi();

    int wiersz = 0;
    for (const auto& [id, nazwa] : tagi) {
        ui->tabela->insertRow(wiersz);
        ui->tabela->setItem(wiersz, 0, new QTableWidgetItem(QString::number(id)));
        ui->tabela->setItem(wiersz, 1, new QTableWidgetItem(nazwa));
        wiersz++;
    }
}

void TagiDialog::onBtnDodajClicked()
{
    bool ok = false;
    const QString nazwa = QInputDialog::getText(
        this, "Nowy tag", "Nazwa:", QLineEdit::Normal, "", &ok
    ).trimmed();

    if (!ok || nazwa.isEmpty()) return;

    // dodajTag zwraca id nowego rekordu lub -1 przy błędzie (np. duplikat nazwy).
    if (appController.dodajTag(nazwa) <= 0) {
        QMessageBox::warning(this, "Błąd", "Nie udało się dodać tagu (może już istnieje).");
        return;
    }

    wypelnijTabele();
}

void TagiDialog::onBtnEdytujClicked()
{
    const int wiersz = ui->tabela->currentRow();
    if (wiersz < 0) {
        QMessageBox::warning(this, "Brak wyboru", "Zaznacz tag na liście.");
        return;
    }

    const int     idTagu      = ui->tabela->item(wiersz, 0)->text().toInt();
    const QString obecnaNazwa = ui->tabela->item(wiersz, 1)->text();

    bool ok = false;
    const QString nowaNazwa = QInputDialog::getText(
        this, "Edytuj tag", "Nazwa:", QLineEdit::Normal, obecnaNazwa, &ok
    ).trimmed();

    if (!ok || nowaNazwa.isEmpty()) return;

    if (appController.aktualizujTag(idTagu, nowaNazwa)) {
        wypelnijTabele();
    } else {
        QMessageBox::warning(this, "Błąd", "Nie udało się zaktualizować tagu.");
    }
}

void TagiDialog::onBtnUsunClicked()
{
    const int wiersz = ui->tabela->currentRow();
    if (wiersz < 0) return;

    const int     idTagu = ui->tabela->item(wiersz, 0)->text().toInt();
    const QString nazwa  = ui->tabela->item(wiersz, 1)->text();

    // Tagi nie pytają "co z zawartością" — usunięcie tagu kasuje tylko powiązania
    // w tabeli multimedia_tagi, same multimedia pozostają nietknięte.
    const auto odp = QMessageBox::question(
        this,
        "Usuń tag",
        QString("Czy na pewno usunąć tag \"%1\"?\nPowiązania z mediami zostaną usunięte.").arg(nazwa),
        QMessageBox::Yes | QMessageBox::No
    );
    if (odp != QMessageBox::Yes) return;

    if (appController.usunTag(idTagu)) {
        wypelnijTabele();
    } else {
        QMessageBox::warning(this, "Błąd", "Nie udało się usunąć tagu.");
    }
}
