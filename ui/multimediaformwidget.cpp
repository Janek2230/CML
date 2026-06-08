#include "multimediaformwidget.h"
#include "ui_multimediaformwidget.h"

#include <QMessageBox>
#include <QTimer>
#include <QInputDialog>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <utility>

namespace {
// Rola węzła comboboxa kategorii: trzyma nazwę jednostki (np. "strony"),
// używaną do etykiety "Cel (jednostka):".
constexpr int ROLA_JEDNOSTKA = Qt::UserRole + 1;
}

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

void MultimediaFormWidget::przygotujFormularz(int idMedium, int idDomyslnejKategorii, int idDomyslnejPlatformy) {
    czyTrybEdycji = (idMedium != -1);
    idEdytowanegoMedium = idMedium;

    uzupelnijComboBoxy();
    uzupelnijTagi();
    if (!czyTrybEdycji) {
        ui->lblInfo->setText("Dodaj nowe medium");
        ui->btnPotwierdzDodaj->setText("Dodaj do biblioteki");
        ui->editNowyTytul->clear();
        ui->spinNowyCel->setValue(1);
        ui->spinNowaOcena->setValue(0);
        ui->spinNowyRok->setValue(0);
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
        ui->lblInfo->setText("Edytuj medium");
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


void MultimediaFormWidget::obsluzSzybkiTag() {
    bool ok;
    QString nazwa = QInputDialog::getText(this, "Nowy Tag", "Podaj nazwę tagu:", QLineEdit::Normal, "", &ok);
    if (ok && !nazwa.trimmed().isEmpty()) {
        if (appController.dodajTag(nazwa.trimmed()) > 0) {
            uzupelnijTagi();
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
    int idDoTagow = -1;

    if (czyTrybEdycji) {
        sukces = appController.aktualizujDaneMedium(idEdytowanegoMedium, tytul, idKat, idPlat, cel, rok, tworca);
        idDoTagow = idEdytowanegoMedium;
    } else {
        idDoTagow = appController.dodajNoweMedium(tytul, idKat, idPlat, cel, rok, tworca);
        sukces = (idDoTagow > 0);
    }

    if (sukces) {
        // Zbieramy ID zaznaczonych tagów: przechodzimy po wszystkich pozycjach listy i dla
        // każdej z zaznaczonym checkboxem (Qt::Checked) odczytujemy ID tagu schowane pod rolą
        // Qt::UserRole (ustawioną w uzupelnijTagi)
        QList<int> wybraneTagi;
        for (int i = 0; i < ui->listTagi->count(); ++i) {
            if (ui->listTagi->item(i)->checkState() == Qt::Checked) {
                wybraneTagi.append(ui->listTagi->item(i)->data(Qt::UserRole).toInt());
            }
        }
        appController.ustawTagiDlaMedium(idDoTagow, wybraneTagi);

        // Odświeżamy listy w comboboxach, ale uzupelnijComboBoxy() buduje je od zera i przy
        // okazji resetuje bieżący wybór (wraca na pozycję 0). Żeby użytkownik nie tracił
        // wybranej kategorii/platformy po zapisie, zapamiętujemy ich ID
        int zapisaneIdKat = ui->comboNowaKategoria->currentData().toInt();
        int zapisaneIdPlat = ui->comboNowyTyp->currentData().toInt();
        uzupelnijComboBoxy();
        int idxKat = ui->comboNowaKategoria->findData(zapisaneIdKat);
        if (idxKat != -1) ui->comboNowaKategoria->setCurrentIndex(idxKat);
        int idxPlat = ui->comboNowyTyp->findData(zapisaneIdPlat);
        if (idxPlat != -1) ui->comboNowyTyp->setCurrentIndex(idxPlat);

        ui->lblKomunikatFormularza->setStyleSheet("color: green; font-weight: bold;");
        ui->lblKomunikatFormularza->setText(czyTrybEdycji ? "Zaktualizowano: " + tytul : "Dodano do biblioteki: " + tytul);
        // W trybie dodawania czyścimy pola, żeby od razu wpisać kolejną pozycję.
        // W trybie edycji zostawiamy wartości — użytkownik dalej widzi to, co właśnie zapisał.
        if (!czyTrybEdycji) {
            ui->editNowyTytul->clear();
            ui->spinNowyRok->setValue(0);
            ui->editNowyTworca->clear();
        }
        QTimer::singleShot(4000, this, [this]() { ui->lblKomunikatFormularza->clear(); });
    } else {
        ui->lblKomunikatFormularza->setStyleSheet("color: red; font-weight: bold;");
        ui->lblKomunikatFormularza->setText("Błąd bazy danych!");
        QTimer::singleShot(4000, this, [this]() { ui->lblKomunikatFormularza->clear(); });
    }
}

// Przebudowuje od zera oba comboboxy (kategorii i platform) na podstawie aktualnych słowników z bazy
void MultimediaFormWidget::uzupelnijComboBoxy() {
    ui->comboNowaKategoria->clear();
    ui->comboNowyTyp->clear();

    // Placeholder kategorii: data 0 = brak wyboru, pod ROLA_JEDNOSTKA tekst zastępczy, żeby
    // etykieta celu miała co czytać, dopóki nic nie wybrano.
    ui->comboNowaKategoria->addItem("--- Wybierz kategorię ---", 0);
    ui->comboNowaKategoria->setItemData(0, "jednostka", ROLA_JEDNOSTKA);

    auto kategorie = appController.pobierzKategorie();
    auto jednostki = appController.pobierzSlownikJednostek();
    for (const auto& kat : std::as_const(kategorie)) {
        if (kat.first != 0) {   // 0 to "brak wyboru", realne id z bazy startują od 1
            ui->comboNowaKategoria->addItem(kat.second, kat.first);
            int idx = ui->comboNowaKategoria->count() - 1;
            QString jedn = jednostki.value(kat.first, "Wartość");   // "Wartość" gdy kategoria nie ma jednostki
            ui->comboNowaKategoria->setItemData(idx, jedn, ROLA_JEDNOSTKA);
        }
    }

    // Platformy — bez jednostek, tylko placeholder + nazwy.
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
        QString jednostka = ui->comboNowaKategoria->itemData(indeks, ROLA_JEDNOSTKA).toString();
        ui->labJednostka->setText(jednostka.isEmpty() ? "Cel:" : "Cel (" + jednostka + "):");
    }
}

// Buduje listę tagów do zaznaczania: KAŻDY istniejący tag staje się pozycją z checkboxem.
// W trybie edycji tagi już przypisane do medium są od razu zaznaczone, reszta odznaczona.
void MultimediaFormWidget::uzupelnijTagi() {
    ui->listTagi->clear();
    const auto wszystkieTagi = appController.pobierzTagi();

    // Mapa: id medium -> nazwy jego tagów. Wyciągamy listę dla edytowanego medium, żeby wiedzieć,
    // które pozycje zaznaczyć (w trybie dodawania ta lista jest po prostu pusta).
    QMap<int, QStringList> przypisane = appController.pobierzPrzypisaniaTagow();
    QStringList tagiTegoMedium = przypisane.value(idEdytowanegoMedium);

    for (const auto& tag : wszystkieTagi) {
        QListWidgetItem* item = new QListWidgetItem(tag.second, ui->listTagi);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);   // włącza checkbox przy pozycji
        item->setData(Qt::UserRole, tag.first);                    // ID tagu pod rolą UserRole (czyta je zapis)

        // Zaznaczamy tylko w edycji i tylko tagi już przypisane.
        if (czyTrybEdycji && tagiTegoMedium.contains(tag.second)) {
            item->setCheckState(Qt::Checked);
        } else {
            item->setCheckState(Qt::Unchecked);
        }
    }
}


