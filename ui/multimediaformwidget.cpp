#include "multimediaformwidget.h"
#include "ui_multimediaformwidget.h"

#include <QMessageBox>
#include <QTimer>
#include <QInputDialog>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <utility>

MultimediaFormWidget::MultimediaFormWidget(AppController& controller, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MultimediaFormWidget),
    appController(controller)
{
    ui->setupUi(this);

    connect(ui->btnAnulujDodawanie, &QPushButton::clicked, this, [this]() {
        czyTrybEdycji = false;
        idEdytowanegoMedium = -1;
        emit formularzAnulowany();
    });

    connect(ui->comboNowaKategoria, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MultimediaFormWidget::obsluzZmianeKategorii);

    connect(ui->btnPotwierdzDodaj, &QPushButton::clicked, this, &MultimediaFormWidget::obsluzPotwierdzDodaj);

    connect(ui->btnDodajPlatforme, &QPushButton::clicked, this, &MultimediaFormWidget::obsluzSzybkaPlatforma);
    connect(ui->btnDodajKategorie, &QPushButton::clicked, this, &MultimediaFormWidget::obsluzSzybkaKategoria);

    connect(ui->btnDodajTag, &QPushButton::clicked, this, &MultimediaFormWidget::obsluzSzybkiTag);
}

MultimediaFormWidget::~MultimediaFormWidget()
{
    delete ui;
}

void MultimediaFormWidget::obsluzSzybkiTag() {
    bool ok;
    QString nazwa = QInputDialog::getText(this, "Nowy Tag", "Podaj nazwę tagu:", QLineEdit::Normal, "", &ok);
    if (ok && !nazwa.trimmed().isEmpty()) {
        if (appController.dodajTag(nazwa.trimmed()) > 0) {
            uzupelnijTagi();
        }
    }
}


void MultimediaFormWidget::przygotujFormularz(int idMedium, int idDomyslnejKategorii, int idDomyslnejPlatformy) {
    // Stan trybu ustawiamy NAJPIERW — uzupelnijTagi() poniżej zależy od czyTrybEdycji
    // oraz idEdytowanegoMedium, więc muszą dotyczyć TEGO wywołania. Inaczej tagi odtwarzają
    // się z opóźnieniem o jedno przełączenie między "Dodaj" a "Edytuj".
    czyTrybEdycji = (idMedium != -1);
    idEdytowanegoMedium = idMedium;

    uzupelnijComboBoxy();
    uzupelnijTagi();
    if (idMedium == -1) {
        ui->label_6->setText("Dodaj nowe medium");
        ui->btnPotwierdzDodaj->setText("Dodaj do biblioteki");
        ui->editNowyTytul->clear();
        ui->spinNowyCel->setValue(1);
        ui->spinNowaOcena->setValue(0);
        ui->spinNowyRok->setValue(0);   // 0 == "—" (nieznany rok)
        ui->editNowyTworca->clear();
        if (idDomyslnejKategorii != 0) {
            int indeksKat = ui->comboNowaKategoria->findData(idDomyslnejKategorii);
            if (indeksKat != -1) ui->comboNowaKategoria->setCurrentIndex(indeksKat);
        }
        if (idDomyslnejPlatformy != 0) {
            int indeksPlat = ui->comboNowyTyp->findData(idDomyslnejPlatformy);
            if (indeksPlat != -1) ui->comboNowyTyp->setCurrentIndex(indeksPlat);
        }
    } else {
        ui->label_6->setText("Edytuj medium");
        ui->btnPotwierdzDodaj->setText("Zapisz zmiany");
        for (const auto& m : appController.pobierzWszystkieMultimedia()) {
            if (m->id == idMedium) {
                ui->editNowyTytul->setText(m->tytul);
                ui->spinNowyCel->setValue(m->postep.docelowa);
                ui->spinNowaOcena->setValue(m->ocena);
                ui->spinNowyRok->setValue(m->rokWydania);
                ui->editNowyTworca->setText(m->tworcy);
                int idxKat = ui->comboNowaKategoria->findData(m->idKategorii);
                ui->comboNowaKategoria->setCurrentIndex(idxKat != -1 ? idxKat : 0);
                int idxPlat = ui->comboNowyTyp->findData(m->idPlatformy);
                ui->comboNowyTyp->setCurrentIndex(idxPlat != -1 ? idxPlat : 0);
                break;
            }
        }
    }
}

void MultimediaFormWidget::obsluzPotwierdzDodaj() {
    QString tytul = ui->editNowyTytul->text().trimmed();
    if (tytul.isEmpty()) {
        QMessageBox::warning(this, "Błąd", "Tytuł nie może być pusty!");
        return;
    }

    int idKat = ui->comboNowaKategoria->currentData().toInt();
    int idPlat = ui->comboNowyTyp->currentData().toInt();
    int cel = ui->spinNowyCel->value();
    int rok = ui->spinNowyRok->value();                  // 0 == nieznany
    QString tworca = ui->editNowyTworca->text().trimmed();

    bool sukces = false;
    int idDoTagow = -1; // Zmienna na ID do przypisania tagów

    if (czyTrybEdycji) {
        sukces = appController.aktualizujDaneMedium(idEdytowanegoMedium, tytul, idKat, idPlat, cel, rok, tworca);
        idDoTagow = idEdytowanegoMedium;
    } else {
        idDoTagow = appController.dodajNoweMedium(tytul, idKat, idPlat, cel, rok, tworca);
        sukces = (idDoTagow > 0);
    }

    if (sukces) {
        // Zapisujemy tagi dla nowo dodanego lub edytowanego medium.
        QList<int> wybraneTagi;
        for (int i = 0; i < ui->listTagi->count(); ++i) {
            if (ui->listTagi->item(i)->checkState() == Qt::Checked) {
                wybraneTagi.append(ui->listTagi->item(i)->data(Qt::UserRole).toInt());
            }
        }
        appController.ustawTagiDlaMedium(idDoTagow, wybraneTagi);

        // Zapamiętujemy aktualny wybór przed przeładowaniem comboboxów.
        int zapisaneIdKat = ui->comboNowaKategoria->currentData().toInt();
        int zapisaneIdPlat = ui->comboNowyTyp->currentData().toInt();
        uzupelnijComboBoxy();
        int idxKat = ui->comboNowaKategoria->findData(zapisaneIdKat);
        if (idxKat != -1) ui->comboNowaKategoria->setCurrentIndex(idxKat);
        int idxPlat = ui->comboNowyTyp->findData(zapisaneIdPlat);
        if (idxPlat != -1) ui->comboNowyTyp->setCurrentIndex(idxPlat);

        ui->lblKomunikatFormularza->setStyleSheet("color: green; font-weight: bold;");
        ui->lblKomunikatFormularza->setText(czyTrybEdycji ? "Zaktualizowano: " + tytul : "Dodano do biblioteki: " + tytul);
        ui->editNowyTytul->clear();
        ui->spinNowyRok->setValue(0);
        ui->editNowyTworca->clear();
    } else {
        ui->lblKomunikatFormularza->setStyleSheet("color: red; font-weight: bold;");
        ui->lblKomunikatFormularza->setText("Błąd bazy danych!");
        QTimer::singleShot(4000, this, [this]() { ui->lblKomunikatFormularza->clear(); });
    }
}

void MultimediaFormWidget::uzupelnijComboBoxy() {
    ui->comboNowaKategoria->clear();
    ui->comboNowyTyp->clear();

    ui->comboNowaKategoria->addItem("--- Wybierz kategorię ---", 0);
    ui->comboNowaKategoria->setItemData(0, "jednostka", Qt::UserRole + 1);

    auto kategorie = appController.pobierzKategorie();
    auto jednostki = appController.pobierzSlownikJednostek();
    for (const auto& kat : std::as_const(kategorie)) {
        if (kat.first != 0) {
            ui->comboNowaKategoria->addItem(kat.second, kat.first);
            int idx = ui->comboNowaKategoria->count() - 1;
            QString jedn = jednostki.value(kat.first, "Wartość");
            ui->comboNowaKategoria->setItemData(idx, jedn, Qt::UserRole + 1);
        }
    }

    ui->comboNowyTyp->addItem("--- Wybierz platformę ---", 0);
    auto platformy = appController.pobierzPlatformy();
    for (const auto& plat : std::as_const(platformy)) {
        if (plat.first != 0) ui->comboNowyTyp->addItem(plat.second, plat.first);
    }
}

void MultimediaFormWidget::obsluzSzybkaPlatforma() {
    bool ok;
    QString nowaNazwa = QInputDialog::getText(this, "Nowa Platforma", "Podaj nazwę:", QLineEdit::Normal, "", &ok);
    if (ok && !nowaNazwa.trimmed().isEmpty()) {
        int noweId = appController.dodajPlatforme(nowaNazwa.trimmed());
        if (noweId > 0) {
            uzupelnijComboBoxy();
        }
    }
}

void MultimediaFormWidget::obsluzSzybkaKategoria() {
    QDialog dialog(this);
    QFormLayout form(&dialog);
    QLineEdit editNazwa(&dialog);
    QComboBox comboJednostka(&dialog);
    comboJednostka.setEditable(true);
    comboJednostka.addItems(appController.pobierzUnikalneJednostki());
    form.addRow("Nazwa kategorii:", &editNazwa);
    form.addRow("Jednostka:", &comboJednostka);

    QDialogButtonBox panelPrzyciskow(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&panelPrzyciskow);

    connect(&panelPrzyciskow, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&panelPrzyciskow, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted && !editNazwa.text().trimmed().isEmpty()) {
        QString jednostka = comboJednostka.currentText().trimmed();
        if (appController.dodajKategorie(editNazwa.text().trimmed(), jednostka)) {
            uzupelnijComboBoxy();
        }
    }
}

void MultimediaFormWidget::obsluzZmianeKategorii(int indeks) {
    if (indeks >= 0) {
        QString jednostka = ui->comboNowaKategoria->itemData(indeks, Qt::UserRole + 1).toString();
        ui->labJednostka->setText(jednostka.isEmpty() ? "Cel:" : "Cel (" + jednostka + "):");
    }
}

void MultimediaFormWidget::uzupelnijTagi() {
    ui->listTagi->clear();
    auto wszystkieTagi = appController.pobierzTagi();

    // Pobieramy przypisania, aby wiedzieć co zaznaczyć w trybie edycji
    QMap<int, QStringList> przypisane = appController.pobierzPrzypisaniaTagow();
    QStringList tagiTegoMedium = przypisane.value(idEdytowanegoMedium);

    for (const auto& tag : wszystkieTagi) {
        QListWidgetItem* item = new QListWidgetItem(tag.second, ui->listTagi);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setData(Qt::UserRole, tag.first);

        if (czyTrybEdycji && tagiTegoMedium.contains(tag.second)) {
            item->setCheckState(Qt::Checked);
        } else {
            item->setCheckState(Qt::Unchecked);
        }
    }
}


