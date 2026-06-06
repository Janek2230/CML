#include "navigationpanelwidget.h"
#include "ui_navigationpanelwidget.h"

#include <QDialog>
#include <QFormLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QLocale>

NavigationPanelWidget::NavigationPanelWidget(AppController& controller, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NavigationPanelWidget),
    appController(controller)
{
    ui->setupUi(this);

    ui->kategorie->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->kategorie->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // Przebudowanie drzewa przy każdej zmianie trybu grupowania.
    connect(ui->comboGrupowanie, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &NavigationPanelWidget::odswiezDrzewo);

    connect(ui->btnPowrot, &QPushButton::clicked, this, [this]() {
        emit zadaniePowrotuDoDashboardu();
    });

    connect(ui->btnDodajMedium, &QPushButton::clicked, this, [this]() {
        emit zadanieDodaniaMedium(0, 0);
    });

    connect(ui->kategorie, &QTreeWidget::itemClicked,
            this, &NavigationPanelWidget::obsluzWyborElementuDrzewa);
    connect(ui->wyszukiwarka, &QLineEdit::textChanged,
            this, &NavigationPanelWidget::obsluzWyszukiwanie);
    connect(ui->kategorie, &QTreeWidget::customContextMenuRequested,
            this, &NavigationPanelWidget::pokazMenuDrzewa);

    zaladujDaneDoDrzewa();
}

NavigationPanelWidget::~NavigationPanelWidget()
{
    delete ui;
}

void NavigationPanelWidget::odswiezDrzewo()
{
    zaladujDaneDoDrzewa();
}

void NavigationPanelWidget::obsluzWyborElementuDrzewa(QTreeWidgetItem *item, int /*kolumna*/)
{
    if (!item || item->parent() == nullptr) {
        // Kliknięcie w węzeł-folder (gałąź) zwija/rozwija go i wraca do dashboardu.
        if (item) item->setExpanded(!item->isExpanded());
        emit zadaniePowrotuDoDashboardu();
        return;
    }
    // Kliknięcie w liść — emitujemy ID przechowywanego medium.
    int idWybranegoElementu = item->data(0, Qt::UserRole).toInt();
    emit zadaniePokazaniaSzczegolow(idWybranegoElementu);
}

void NavigationPanelWidget::obsluzWyszukiwanie(const QString &tekst)
{
    if (ui->kategorie->topLevelItemCount() == 0) return;

    for (int i = 0; i < ui->kategorie->topLevelItemCount(); ++i) {
        QTreeWidgetItem *kategoria = ui->kategorie->topLevelItem(i);
        bool maPasujaceElementy = false;

        for (int j = 0; j < kategoria->childCount(); ++j) {
            QTreeWidgetItem *dziecko = kategoria->child(j);
            const bool pasuje = dziecko->text(0).contains(tekst, Qt::CaseInsensitive);
            dziecko->setHidden(!pasuje);
            if (pasuje) maPasujaceElementy = true;
        }

        if (tekst.isEmpty()) {
            kategoria->setHidden(false);
        } else {
            kategoria->setHidden(!maPasujaceElementy);
            if (maPasujaceElementy) kategoria->setExpanded(true);
        }
    }
}

void NavigationPanelWidget::zaladujDaneDoDrzewa()
{
    ui->kategorie->clear();
    listaMultimediow = appController.pobierzWszystkieMultimedia();

    const int trybGrupowania = ui->comboGrupowanie->currentIndex();

    // Pobieramy tylko te dane słownikowe, których aktualnie potrzebujemy.
    QMap<int, QString>          slownikKategorii;
    QList<QPair<int, QString>>  listaPlatform;
    QMap<int, QStringList>      mapaTagow;
    QMap<int, QString>          mapaTypowNosnika; // id_platformy → typ_nosnika (tryb 5)

    if (trybGrupowania == 0) slownikKategorii = appController.pobierzSlownikKategorii();
    if (trybGrupowania == 2) listaPlatform    = appController.pobierzPlatformy();
    if (trybGrupowania == 4) mapaTagow        = appController.pobierzPrzypisaniaTagow();
    if (trybGrupowania == 5) {
        for (const auto& p : appController.pobierzPelnePlatformy())
            mapaTypowNosnika.insert(p.id, p.typNosnika);
    }

    // Mapa istniejących węzłów-folderów, by nie tworzyć duplikatów.
    QMap<QString, QTreeWidgetItem*> wezlyGlowne;

    // Etap 1: Tworzymy foldery dla trybów ze stałą listą grup (Kategoria, Platforma, Tagi).
    // Dla Status i Data foldery tworzone są dynamicznie podczas przetwarzania multimediów.
    if (trybGrupowania == 0) {
        for (const auto& kat : appController.pobierzKategorie()) {
            QTreeWidgetItem *wezel = new QTreeWidgetItem(ui->kategorie);
            wezel->setText(0, kat.second);
            wezel->setData(0, Qt::UserRole, kat.first);
            wezlyGlowne.insert(kat.second, wezel);
        }
    } else if (trybGrupowania == 2) {
        for (const auto& plat : listaPlatform) {
            QTreeWidgetItem *wezel = new QTreeWidgetItem(ui->kategorie);
            wezel->setText(0, plat.second);
            wezel->setData(0, Qt::UserRole, plat.first);
            wezlyGlowne.insert(plat.second, wezel);
        }
    } else if (trybGrupowania == 4) {
        // Ładujemy wszystkie tagi z bazy, żeby puste tagi też pojawiły się w drzewie.
        for (const auto& tag : appController.pobierzTagi()) {
            QTreeWidgetItem *wezel = new QTreeWidgetItem(ui->kategorie);
            wezel->setText(0, tag.second);
            wezel->setData(0, Qt::UserRole, tag.first);
            wezlyGlowne.insert(tag.second, wezel);
        }
    }

    // Etap 2: Przypisujemy każde medium do odpowiedniego folderu.
    for (const auto& medium : std::as_const(listaMultimediow)) {

        // Tagi to relacja wiele-do-wielu — jedno medium może trafić do kilku folderów.
        if (trybGrupowania == 4) {
            QStringList tagiMedium = mapaTagow.value(medium->id);
            if (tagiMedium.isEmpty()) tagiMedium << "Brak tagów";

            for (const QString& nazwaGrupy : tagiMedium) {
                if (!wezlyGlowne.contains(nazwaGrupy)) {
                    QTreeWidgetItem *nowyWezel = new QTreeWidgetItem(ui->kategorie);
                    nowyWezel->setText(0, nazwaGrupy);
                    wezlyGlowne.insert(nazwaGrupy, nowyWezel);
                }
                QTreeWidgetItem *item = new QTreeWidgetItem(wezlyGlowne[nazwaGrupy]);
                item->setText(0, medium->tytul);
                item->setData(0, Qt::UserRole, medium->id);
            }
            continue;
        }

        // Pozostałe tryby — relacja jeden-do-jednego z folderem.
        QString nazwaGrupy;
        switch (trybGrupowania) {
        case 0:
            nazwaGrupy = slownikKategorii.value(medium->idKategorii, "Brak kategorii");
            break;
        case 1:
            nazwaGrupy = medium->status;
            break;
        case 2:
            nazwaGrupy = "Nieznana platforma";
            for (const auto& plat : listaPlatform) {
                if (plat.first == medium->idPlatformy) { nazwaGrupy = plat.second; break; }
            }
            break;
        case 3: {
            QLocale polski(QLocale::Polish, QLocale::Poland);
            QDate data = medium->dataDodania.date();
            nazwaGrupy = data.isValid()
                ? QString("%1 %2").arg(polski.standaloneMonthName(data.month())).arg(data.year()).toUpper()
                : "BRAK DATY";
            break;
        }
        case 5: {
            const QString typ = mapaTypowNosnika.value(medium->idPlatformy);
            nazwaGrupy = typ.isEmpty() ? "Nieznany typ nośnika" : typ;
            break;
        }
        default:
            break;
        }

        if (!wezlyGlowne.contains(nazwaGrupy)) {
            QTreeWidgetItem *nowyWezel = new QTreeWidgetItem(ui->kategorie);
            nowyWezel->setText(0, nazwaGrupy);
            if (trybGrupowania == 0)
                nowyWezel->setData(0, Qt::UserRole, medium->idKategorii);
            else if (trybGrupowania == 2)
                nowyWezel->setData(0, Qt::UserRole, medium->idPlatformy);
            wezlyGlowne.insert(nazwaGrupy, nowyWezel);
        }

        QTreeWidgetItem *item = new QTreeWidgetItem(wezlyGlowne[nazwaGrupy]);
        item->setText(0, medium->tytul);
        item->setData(0, Qt::UserRole, medium->id);
    }

    ui->kategorie->expandAll();
}

void NavigationPanelWidget::pokazMenuDrzewa(const QPoint &pos)
{
    QList<QTreeWidgetItem*> zaznaczone = ui->kategorie->selectedItems();
    if (zaznaczone.isEmpty()) return;

    // Zbieramy ID tylko z liści (elementów z rodzicem) — węzły-foldery ignorujemy.
    QList<int> wybraneId;
    for (auto *item : zaznaczone) {
        if (item->parent() != nullptr) {
            wybraneId.append(item->data(0, Qt::UserRole).toInt());
        }
    }
    if (wybraneId.isEmpty()) return;

    QMenu menu(this);

    if (wybraneId.size() == 1) {
        QAction *akcjaEdytuj = menu.addAction("Edytuj wpis");
        connect(akcjaEdytuj, &QAction::triggered, this, [this, id = wybraneId.first()]() {
            emit zadanieEdycjiMedium(id);
        });
        menu.addSeparator();
    }

    // Podmenu zmiany kategorii — budowane dynamicznie z aktualnej listy kategorii.
    QMenu *podmenuKategorii = menu.addMenu("Zmień kategorię na...");
    const auto mapaKategorii = appController.pobierzSlownikKategorii();
    for (auto it = mapaKategorii.begin(); it != mapaKategorii.end(); ++it) {
        const int idKat = it.key();
        QAction *akt = podmenuKategorii->addAction(it.value());
        connect(akt, &QAction::triggered, this, [this, wybraneId, idKat]() {
            if (!appController.zmienKategorieWielu(wybraneId, idKat)) {
                QMessageBox::critical(this, "Błąd", "Nie udało się zmienić kategorii w bazie danych.");
            }
            // Drzewo odświeży się automatycznie przez sygnał daneZmienione().
        });
    }

    menu.addSeparator();

    const QString tekstUsun = (wybraneId.size() > 1)
        ? QString("Usuń zaznaczone elementy (%1)").arg(wybraneId.size())
        : "Usuń z biblioteki";

    QAction *akcjaUsun = menu.addAction(tekstUsun);
    connect(akcjaUsun, &QAction::triggered, this, [this, wybraneId]() {
        const QString pytanie = (wybraneId.size() > 1)
            ? QString("Czy na pewno chcesz usunąć %1 elementów?").arg(wybraneId.size())
            : "Czy na pewno chcesz usunąć ten wpis?";

        if (QMessageBox::question(this, "Potwierdzenie", pytanie,
                                  QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

        if (wybraneId.size() == 1) {
            appController.usunMedium(wybraneId.first());
        } else {
            appController.usunWieleMultimediow(wybraneId);
        }
        emit zadaniePowrotuDoDashboardu();
    });

    menu.exec(ui->kategorie->viewport()->mapToGlobal(pos));
}
