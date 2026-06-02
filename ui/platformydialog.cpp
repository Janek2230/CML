#include "platformydialog.h"
#include "ui_platformydialog.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>

PlatformyDialog::PlatformyDialog(AppController& controller, QWidget *parent)
    : QDialog(parent), ui(new Ui::PlatformyDialog), appController(controller)
{
    ui->setupUi(this);

    ui->tabela->hideColumn(0);
    ui->tabela->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    wypelnijTabele();

    connect(ui->btnDodaj,    &QPushButton::clicked, this, &PlatformyDialog::onBtnDodajClicked);
    connect(ui->btnEdytuj,   &QPushButton::clicked, this, &PlatformyDialog::onBtnEdytujClicked);
    connect(ui->btnUsun,     &QPushButton::clicked, this, &PlatformyDialog::onBtnUsunClicked);
    connect(ui->btnZamknij,  &QPushButton::clicked, this, &QDialog::accept);
}

PlatformyDialog::~PlatformyDialog()
{
    delete ui;
}

void PlatformyDialog::wypelnijTabele()
{
    ui->tabela->setRowCount(0);
    const auto platformy = appController.pobierzPelnePlatformy();

    int wiersz = 0;
    for (const auto& p : platformy) {
        ui->tabela->insertRow(wiersz);
        ui->tabela->setItem(wiersz, 0, new QTableWidgetItem(QString::number(p.id)));
        ui->tabela->setItem(wiersz, 1, new QTableWidgetItem(p.nazwa));
        ui->tabela->setItem(wiersz, 2, new QTableWidgetItem(p.typNosnika));
        wiersz++;
    }
}

// Wspólna lista typów nośnika używana przy dodawaniu i edycji platformy.
static QStringList dostepneTypyNosnika() {
    return {"Cyfrowy", "Fizyczny", "Streaming", "Papier", "E-book"};
}

void PlatformyDialog::onBtnDodajClicked()
{
    bool ok = false;
    const QString nazwa = QInputDialog::getText(
        this, "Nowa platforma", "Nazwa:", QLineEdit::Normal, "", &ok
    ).trimmed();

    if (!ok || nazwa.isEmpty()) return;

    const QString typ = QInputDialog::getItem(
        this, "Typ nośnika", "Wybierz typ nośnika:", dostepneTypyNosnika(), 0, false, &ok
    );
    if (!ok) return;

    if (appController.dodajPlatforme(nazwa, typ) <= 0) {
        QMessageBox::warning(this, "Błąd", "Nie udało się dodać platformy.");
        return;
    }

    wypelnijTabele();
}

void PlatformyDialog::onBtnEdytujClicked()
{
    const int wiersz = ui->tabela->currentRow();
    if (wiersz < 0) {
        QMessageBox::warning(this, "Brak wyboru", "Zaznacz platformę na liście.");
        return;
    }

    const int     idPlat      = ui->tabela->item(wiersz, 0)->text().toInt();
    const QString obecnaNazwa = ui->tabela->item(wiersz, 1)->text();
    const QString obecnyTyp   = ui->tabela->item(wiersz, 2) ? ui->tabela->item(wiersz, 2)->text() : QString();

    bool ok = false;
    const QString nowaNazwa = QInputDialog::getText(
        this, "Edytuj platformę", "Nazwa:", QLineEdit::Normal, obecnaNazwa, &ok
    ).trimmed();

    if (!ok || nowaNazwa.isEmpty()) return;

    // Lista typów z aktualnie ustawionym typem zaznaczonym domyślnie.
    const QStringList typy = dostepneTypyNosnika();
    const int indeksTypu = qMax(0, typy.indexOf(obecnyTyp));
    const QString nowyTyp = QInputDialog::getItem(
        this, "Typ nośnika", "Wybierz typ nośnika:", typy, indeksTypu, false, &ok
    );
    if (!ok) return;

    if (appController.aktualizujPlatforme(idPlat, nowaNazwa, nowyTyp)) {
        wypelnijTabele();
    }
}

void PlatformyDialog::onBtnUsunClicked()
{
    const int wiersz = ui->tabela->currentRow();
    if (wiersz < 0) return;

    const int     idPlat = ui->tabela->item(wiersz, 0)->text().toInt();
    const QString nazwa  = ui->tabela->item(wiersz, 1)->text();

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Usuwanie platformy");
    msgBox.setText("Usuwasz platformę: <b>" + nazwa + "</b><br>Co robimy z przypisaną zawartością?");

    // Musimy zachować wskaźniki do przycisków, żeby później sprawdzić który
    // został kliknięty przez msgBox.clickedButton().
    QPushButton *btnPrzenies     = msgBox.addButton("Przenieś do 'Brak'", QMessageBox::ActionRole);
    QPushButton *btnUsunWszystko = msgBox.addButton("Usuń wszystko",      QMessageBox::DestructiveRole);
    msgBox.addButton("Anuluj", QMessageBox::RejectRole);

    msgBox.exec();

    QAbstractButton *clicked = msgBox.clickedButton();
    if (clicked != btnPrzenies && clicked != btnUsunWszystko) return;

    const bool usunZDetalami = (clicked == btnUsunWszystko);
    if (appController.usunPlatforme(idPlat, usunZDetalami)) {
        wypelnijTabele();
    }
}
