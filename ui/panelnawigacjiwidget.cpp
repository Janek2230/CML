#include "panelnawigacjiwidget.h"
#include "ui_panelnawigacjiwidget.h"

#include <QDialog>
#include <QFormLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QLocale>

PanelNawigacjiWidget::PanelNawigacjiWidget(DatabaseManager& db, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PanelNawigacjiWidget),
    dbManager(db)
{
    ui->setupUi(this);

    ui->comboGrupowanie->addItems({"Kategoria", "Status", "Platforma", "Data dodania"});
    ui->kategorie->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->kategorie->setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(ui->comboGrupowanie, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        zaladujDaneDoDrzewa();
    });

    connect(ui->btnPowrot, &QPushButton::clicked, this, [this]() {
        emit zadaniePowrotuDoDashboardu();
    });

    connect(ui->btnDodajMedium, &QPushButton::clicked, this, [this]() {
        emit zadanieDodaniaMedium(0, 0);
    });

    connect(ui->kategorie, &QTreeWidget::itemClicked, this, &PanelNawigacjiWidget::onWybieranieElementuDrzewa);
    connect(ui->wyszukiwarka, &QLineEdit::textChanged, this, &PanelNawigacjiWidget::onWyszukiwanie);
    connect(ui->kategorie, &QTreeWidget::customContextMenuRequested, this, &PanelNawigacjiWidget::pokazMenuDrzewa);

    zaladujDaneDoDrzewa();
}

PanelNawigacjiWidget::~PanelNawigacjiWidget()
{
    delete ui;
}

void PanelNawigacjiWidget::odswiezDrzewo() {
    zaladujDaneDoDrzewa();
}

void PanelNawigacjiWidget::onWybieranieElementuDrzewa(QTreeWidgetItem *item, int column) {
    if (!item || item->parent() == nullptr) {
        if (item) item->setExpanded(!item->isExpanded());
        emit zadaniePowrotuDoDashboardu();
        return;
    }
    int idWybranegoElementu = item->data(0, Qt::UserRole).toInt();
    emit zadaniePokazaniaSzczegolow(idWybranegoElementu);
}

void PanelNawigacjiWidget::onWyszukiwanie(const QString &text) {
    if (ui->kategorie->topLevelItemCount() == 0) return;
    for (int i = 0; i < ui->kategorie->topLevelItemCount(); ++i) {
        QTreeWidgetItem *kategoria = ui->kategorie->topLevelItem(i);
        bool maPasujaceElementy = false;
        for (int j = 0; j < kategoria->childCount(); ++j) {
            QTreeWidgetItem *dziecko = kategoria->child(j);
            if (dziecko->text(0).contains(text, Qt::CaseInsensitive)) {
                dziecko->setHidden(false);
                maPasujaceElementy = true;
            } else {
                dziecko->setHidden(true);
            }
        }
        if (text.isEmpty()) {
            kategoria->setHidden(false);
        } else {
            kategoria->setHidden(!maPasujaceElementy);
            if (maPasujaceElementy) kategoria->setExpanded(true);
        }
    }
}

void PanelNawigacjiWidget::zaladujDaneDoDrzewa() {
    ui->kategorie->clear();
    listaMultimediow = dbManager.getAllMultimedia();

    int trybGrupowania = ui->comboGrupowanie->currentIndex();
    QMap<int, QString> slownikKategorii;
    QList<QPair<int, QString>> listaPlatform;
    if (trybGrupowania == 0) slownikKategorii = dbManager.getCategories();
    if (trybGrupowania == 2) listaPlatform = dbManager.pobierzPlatformy();

    QMap<QString, QTreeWidgetItem*> wezlyGlowne;

    if (trybGrupowania == 0) {
        auto q = dbManager.pobierzKategorie();
        for(const auto& kat : q) {
            QString nazwa = kat.second;
            QTreeWidgetItem *wezel = new QTreeWidgetItem(ui->kategorie);
            wezel->setText(0, nazwa);
            wezel->setData(0, Qt::UserRole, kat.first);
            wezlyGlowne.insert(nazwa, wezel);
        }
    } else if (trybGrupowania == 2) {
        for (const auto& plat : listaPlatform) {
            QString nazwa = plat.second;
            QTreeWidgetItem *wezel = new QTreeWidgetItem(ui->kategorie);
            wezel->setText(0, nazwa);
            wezel->setData(0, Qt::UserRole, plat.first);
            wezlyGlowne.insert(nazwa, wezel);
        }
    }

    for (const auto& medium : std::as_const(listaMultimediow)) {
        QString nazwaGrupy;
        switch (trybGrupowania) {
        case 0: nazwaGrupy = slownikKategorii.value(medium->getIdKategorii(), "Brak kategorii"); break;
        case 1: nazwaGrupy = medium->getStatus(); break;
        case 2:
            nazwaGrupy = "Nieznana platforma";
            for (const auto& plat : listaPlatform) {
                if (plat.first == medium->getIdPlatformy()) { nazwaGrupy = plat.second; break; }
            }
            break;
        case 3:
            QLocale polski(QLocale::Polish, QLocale::Poland);
            QDate data = medium->getDataDodania().date();
            if (data.isValid()) {
                QString miesiac = polski.standaloneMonthName(data.month());
                nazwaGrupy = QString("%1 %2").arg(miesiac).arg(data.year()).toUpper();
            } else {
                nazwaGrupy = "BRAK DATY";
            }
            break;
        }

        if (!wezlyGlowne.contains(nazwaGrupy)) {
            QTreeWidgetItem *nowyWezel = new QTreeWidgetItem(ui->kategorie);
            nowyWezel->setText(0, nazwaGrupy);
            if (trybGrupowania == 0) {
                nowyWezel->setData(0, Qt::UserRole, medium->getIdKategorii());
            } else if (trybGrupowania == 2) {
                nowyWezel->setData(0, Qt::UserRole, medium->getIdPlatformy());
            }
            wezlyGlowne.insert(nazwaGrupy, nowyWezel);
        }

        QTreeWidgetItem *item = new QTreeWidgetItem(wezlyGlowne[nazwaGrupy]);
        item->setText(0, medium->getTytul());
        item->setData(0, Qt::UserRole, medium->getId());
    }
    ui->kategorie->expandAll();
}

void PanelNawigacjiWidget::pokazMenuDrzewa(const QPoint &pos) {
    QTreeWidgetItem *kliknietyElement = ui->kategorie->itemAt(pos);
    QMenu menu(this);

    QList<QTreeWidgetItem*> wybrane = ui->kategorie->selectedItems();

    if (wybrane.size() > 1) {
        QList<int> wybraneIds;
        for (auto *item : wybrane) {
            if (item->parent() != nullptr) {
                wybraneIds.append(item->data(0, Qt::UserRole).toInt());
            }
        }

        if (!wybraneIds.isEmpty() && ui->comboGrupowanie->currentIndex() == 0) {
            menu.addAction(QString("Przenieś zaznaczone (%1) do innej kategorii...").arg(wybraneIds.size()), this, [this, wybraneIds]() {
                QDialog dialog(this);
                dialog.setWindowTitle("Zbiorcza zmiana kategorii");
                dialog.resize(300, 100);

                QFormLayout form(&dialog);
                QComboBox comboNowaKat(&dialog);
                comboNowaKat.addItem("--- Wybierz nową kategorię ---", 0);
                auto kategorie = dbManager.pobierzKategorie();
                for (const auto& kat : std::as_const(kategorie)) {
                    if (kat.first != 0) comboNowaKat.addItem(kat.second, kat.first);
                }

                form.addRow("Wybierz docelową kategorię:", &comboNowaKat);
                QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
                form.addRow(&buttonBox);

                connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
                connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

                if (dialog.exec() == QDialog::Accepted) {
                    int noweIdKat = comboNowaKat.currentData().toInt();
                    if (noweIdKat == 0) return;

                    if (dbManager.zmienKategorieWielu(wybraneIds, noweIdKat)) {
                        zaladujDaneDoDrzewa();
                        emit drzewoZmieniloBaze();
                    } else {
                        QMessageBox::critical(this, "Błąd", "Nie udało się przenieść elementów.");
                    }
                }
            });
        }

        if (!wybraneIds.isEmpty()) {
            menu.addSeparator();
            menu.addAction(QString("Usuń zaznaczone pozycje (%1)").arg(wybraneIds.size()), this, [this, wybraneIds]() {
                if (QMessageBox::question(this, "Masowe usuwanie", QString("Czy na pewno chcesz usunąć bezpowrotnie %1 pozycji z biblioteki?").arg(wybraneIds.size())) == QMessageBox::Yes) {
                    if (dbManager.usunWieleMultimediow(wybraneIds)) {
                        zaladujDaneDoDrzewa();
                        emit zadaniePowrotuDoDashboardu();
                        emit drzewoZmieniloBaze();
                    }
                }
            });
        }
        if (!menu.isEmpty()) menu.exec(ui->kategorie->mapToGlobal(pos));
        return;
    }

    if (!kliknietyElement) {
        int trybGrupowania = ui->comboGrupowanie->currentIndex();
        if (trybGrupowania == 0) {
            menu.addAction("Dodaj nową kategorię...", this, [this]() {
                QDialog dialog(this);
                dialog.setWindowTitle("Nowa Kategoria");
                QFormLayout form(&dialog);
                QLineEdit editNazwa(&dialog);
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
                            zaladujDaneDoDrzewa();
                        }
                    }
                }
            });
        }
        else if (trybGrupowania == 2) {
            menu.addAction("Dodaj nową platformę...", this, [this]() {
                bool ok;
                QString nazwa = QInputDialog::getText(this, "Nowa Platforma", "Podaj nazwę:", QLineEdit::Normal, "", &ok);
                if (ok && !nazwa.trimmed().isEmpty()) {
                    if (dbManager.dodajPlatforme(nazwa.trimmed()) > 0) zaladujDaneDoDrzewa();
                }
            });
        }
    }
    else if (kliknietyElement->parent() == nullptr) {
        int trybGrupowania = ui->comboGrupowanie->currentIndex();

        if (trybGrupowania == 0) {
            menu.addAction("Dodaj pozycję do tej kategorii", this, [this, kliknietyElement]() {
                int idKategorii = kliknietyElement->data(0, Qt::UserRole).toInt();
                emit zadanieDodaniaMedium(idKategorii, 0);
            });

            if (kliknietyElement->text(0) != "Brak kategorii") {
                menu.addSeparator();
                menu.addAction("Usuń kategorię...", this, [this, kliknietyElement]() {
                    int idKat = kliknietyElement->data(0, Qt::UserRole).toInt();
                    QMessageBox msgBox(this);
                    msgBox.setWindowTitle("Usuwanie");
                    msgBox.setText("Usuwasz kategorię: " + kliknietyElement->text(0));
                    QPushButton *btnPrzenies = msgBox.addButton("Przenieś do 'Brak kategorii'", QMessageBox::ActionRole);
                    QPushButton *btnUsunWszystko = msgBox.addButton("Usuń kategorię i pozycje", QMessageBox::DestructiveRole);
                    QPushButton *btnAnuluj = msgBox.addButton("Anuluj", QMessageBox::RejectRole);
                    msgBox.exec();

                    if (msgBox.clickedButton() == btnAnuluj) return;
                    bool usunPowiazane = (msgBox.clickedButton() == btnUsunWszystko);
                    if (dbManager.usunKategorie(idKat, usunPowiazane)) {
                        zaladujDaneDoDrzewa();
                        emit drzewoZmieniloBaze();
                    }
                });
            }
        }
        else if (trybGrupowania == 2) {
            menu.addAction("Dodaj pozycję do tej platformy", this, [this, kliknietyElement]() {
                int idPlatformy = kliknietyElement->data(0, Qt::UserRole).toInt();
                emit zadanieDodaniaMedium(0, idPlatformy);
            });

            if (kliknietyElement->text(0) != "Nieznana platforma") {
                menu.addSeparator();
                menu.addAction("Usuń platformę...", this, [this, kliknietyElement]() {
                    int idPlat = kliknietyElement->data(0, Qt::UserRole).toInt();
                    QMessageBox msgBox(this);
                    msgBox.setWindowTitle("Usuwanie platformy");
                    msgBox.setText("Usuwasz platformę: " + kliknietyElement->text(0));
                    QPushButton *btnPrzenies = msgBox.addButton("Przenieś do 'Nieznana platforma'", QMessageBox::ActionRole);
                    QPushButton *btnUsunWszystko = msgBox.addButton("Usuń platformę i pozycje", QMessageBox::DestructiveRole);
                    QPushButton *btnAnuluj = msgBox.addButton("Anuluj", QMessageBox::RejectRole);
                    msgBox.exec();

                    if (msgBox.clickedButton() == btnAnuluj) return;
                    bool usunPowiazane = (msgBox.clickedButton() == btnUsunWszystko);
                    if (dbManager.usunPlatforme(idPlat, usunPowiazane)) {
                        zaladujDaneDoDrzewa();
                        emit drzewoZmieniloBaze();
                    }
                });
            }
        }
    }
    else {
        menu.addAction("Edytuj pozycję", this, [this, kliknietyElement]() {
            int idMedium = kliknietyElement->data(0, Qt::UserRole).toInt();
            emit zadanieEdycjiMedium(idMedium);
        });

        int trybGrupowania = ui->comboGrupowanie->currentIndex();
        if (trybGrupowania == 0) {
            menu.addAction("Zmień kategorię", this, [this, kliknietyElement]() {
                int idMedium = kliknietyElement->data(0, Qt::UserRole).toInt();
                int obecneIdKategorii = kliknietyElement->parent()->data(0, Qt::UserRole).toInt();
                QDialog dialog(this);
                QFormLayout form(&dialog);
                QComboBox comboNowaKat(&dialog);
                auto kategorie = dbManager.pobierzKategorie();
                for (const auto& kat : std::as_const(kategorie)) {
                    if (kat.first != 0) comboNowaKat.addItem(kat.second, kat.first);
                }
                int index = comboNowaKat.findData(obecneIdKategorii);
                if (index != -1) comboNowaKat.setCurrentIndex(index);
                form.addRow("Przenieś do:", &comboNowaKat);
                QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
                form.addRow(&buttonBox);
                connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
                connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

                if (dialog.exec() == QDialog::Accepted) {
                    int noweIdKat = comboNowaKat.currentData().toInt();
                    if (noweIdKat != obecneIdKategorii) {
                        if (dbManager.zmienKategorieWielu({idMedium}, noweIdKat)) {
                            zaladujDaneDoDrzewa();
                            emit drzewoZmieniloBaze();
                        }
                    }
                }
            });
        }
        menu.addSeparator();
        menu.addAction("Usuń z biblioteki", this, [this, kliknietyElement]() {
            int id = kliknietyElement->data(0, Qt::UserRole).toInt();
            if (QMessageBox::question(this, "Potwierdzenie", "Czy na pewno usunąć?") == QMessageBox::Yes) {
                if (dbManager.usunMedium(id)) {
                    zaladujDaneDoDrzewa();
                    emit zadaniePowrotuDoDashboardu();
                    emit drzewoZmieniloBaze();
                }
            }
        });
    }
    menu.exec(ui->kategorie->mapToGlobal(pos));
}