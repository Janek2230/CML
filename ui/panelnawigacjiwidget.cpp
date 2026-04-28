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

PanelNawigacjiWidget::PanelNawigacjiWidget(AppController& controller, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PanelNawigacjiWidget),
    appController(controller)
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
    listaMultimediow = appController.pobierzWszystkieMultimedia();

    int trybGrupowania = ui->comboGrupowanie->currentIndex();
    QMap<int, QString> slownikKategorii;
    QList<QPair<int, QString>> listaPlatform;
    if (trybGrupowania == 0) slownikKategorii = appController.getCategories();
    if (trybGrupowania == 2) listaPlatform = appController.pobierzPlatformy();

    QMap<QString, QTreeWidgetItem*> wezlyGlowne;

    if (trybGrupowania == 0) {
        auto q = appController.pobierzKategorie();
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
    QList<QTreeWidgetItem*> zaznaczone = ui->kategorie->selectedItems();
    if (zaznaczone.isEmpty()) return;

    // Filtrujemy tylko te elementy, które są mediami (mają rodzica)
    QList<int> wybraneId;
    for (auto *item : zaznaczone) {
        if (item->parent() != nullptr) {
            wybraneId.append(item->data(0, Qt::UserRole).toInt());
        }
    }

    if (wybraneId.isEmpty()) return;

    QMenu menu(this);

    // 1. Opcja Edycji (tylko dla pojedynczego elementu)
    if (wybraneId.size() == 1) {
        QAction *akcjaEdytuj = menu.addAction("Edytuj wpis");
        connect(akcjaEdytuj, &QAction::triggered, this, [this, id = wybraneId.first()]() {
            emit zadanieEdycjiMedium(id);
        });
        menu.addSeparator();
    }

    // 2. Dynamiczne podmenu "Zmień kategorię na..." (logika z archiwalnego mainwindow.cpp)
    QMenu *submenuKategorii = menu.addMenu("Zmień kategorię na...");
    auto kategorieMap = appController.getCategories();

    for (auto it = kategorieMap.begin(); it != kategorieMap.end(); ++it) {
        int idKat = it.key();
        QString nazwaKat = it.value();

        QAction *akt = submenuKategorii->addAction(nazwaKat);
        connect(akt, &QAction::triggered, this, [this, wybraneId, idKat]() {
            if (appController.zmienKategorieWielu(wybraneId, idKat)) {
                // Po zmianie drzewo odświeży się automatycznie dzięki sygnałowi daneZmienione()
            } else {
                QMessageBox::critical(this, "Błąd", "Nie udało się zmienić kategorii w bazie danych.");
            }
        });
    }

    menu.addSeparator();

    // 3. Usuwanie (Pojedyncze lub Masowe)
    QString tekstUsun = (wybraneId.size() > 1)
                            ? QString("Usuń zaznaczone elementy (%1)").arg(wybraneId.size())
                            : "Usuń z biblioteki";

    QAction *akcjaUsun = menu.addAction(tekstUsun);
    connect(akcjaUsun, &QAction::triggered, this, [this, wybraneId]() {
        QString pyt = (wybraneId.size() > 1)
        ? QString("Czy na pewno chcesz usunąć %1 elementów?").arg(wybraneId.size())
        : "Czy na pewno chcesz usunąć ten wpis?";

        if (QMessageBox::question(this, "Potwierdzenie", pyt, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            if (wybraneId.size() == 1) {
                appController.usunMedium(wybraneId.first());
            } else {
                appController.usunWieleMultimediow(wybraneId);
            }
            emit zadaniePowrotuDoDashboardu();
        }
    });

    menu.exec(ui->kategorie->viewport()->mapToGlobal(pos));
}

