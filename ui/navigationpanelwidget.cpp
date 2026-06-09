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

    // Własne menu kontekstowe (prawy klik) — obsługiwane w pokazMenuDrzewa.
    ui->drzewoMedow->setContextMenuPolicy(Qt::CustomContextMenu);
    // Zaznaczanie wielu pozycji (Ctrl/Shift) — pod akcje grupowe: usuń / zmień kategorię wielu.
    ui->drzewoMedow->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // Przebudowanie drzewa przy każdej zmianie trybu grupowania.
    connect(ui->comboGrupowanie, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &NavigationPanelWidget::odswiezDrzewo);

    connect(ui->btnPowrot, &QPushButton::clicked, this, [this]() {
        emit zadaniePowrotuDoDashboardu();
    });

    connect(ui->btnDodajMedium, &QPushButton::clicked, this, [this]() {
        emit zadanieDodaniaMedium(0, 0);
    });

    connect(ui->drzewoMedow, &QTreeWidget::itemClicked,
            this, &NavigationPanelWidget::obsluzWyborElementuDrzewa);

    //Każd wpisnay symbol w polu wyszukiwania wywołuje funkcję wyszukiwania
    connect(ui->wyszukiwarka, &QLineEdit::textChanged,
            this, &NavigationPanelWidget::obsluzWyszukiwanie);
    connect(ui->drzewoMedow, &QTreeWidget::customContextMenuRequested,
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
    // parent() == nullptr → węzeł najwyższego poziomu, czyli folder-grupa (nie konkretne medium).
    if (!item || item->parent() == nullptr) {
        // Kliknięcie w folder tylko zwija/rozwija go (setExpanded przełącza stan isExpanded) i wraca do dashboardu.
        if (item) item->setExpanded(!item->isExpanded());
        emit zadaniePowrotuDoDashboardu();
        return;
    }
    // Kliknięcie w liść — emitujemy ID przechowywanego medium.
    int idWybranegoElementu = item->data(0, Qt::UserRole).toInt();
    emit zadaniePokazaniaSzczegolow(idWybranegoElementu);
}

// Filtruje drzewo w miejscu (bez przebudowy): ukrywa tytuły niepasujące do tekstu, a foldery
// bez żadnego trafienia chowa w całości. Foldery z trafieniami rozwija. Pusty tekst = pokaż wszystko.
void NavigationPanelWidget::obsluzWyszukiwanie(const QString &tekst)
{
    if (ui->drzewoMedow->topLevelItemCount() == 0) return;

    // Nawigacja po drzewie: topLevelItem(i) to węzły najwyższego poziomu, kategorie (foldery-grupy, np. kategorie),
    // a child(j) ich dzieci (media).
    for (int i = 0; i < ui->drzewoMedow->topLevelItemCount(); ++i) {
        QTreeWidgetItem *kategoria = ui->drzewoMedow->topLevelItem(i);
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
    ui->drzewoMedow->clear();
    const auto listaMultimediow = appController.pobierzWszystkieMultimedia();

    // Tryb grupowania = indeks pozycji w comboGrupowanie. Te numery sterują całą funkcją:
    //   0 = Kategoria, 1 = Status, 2 = Platforma, 3 = Data dodania (miesiąc rok),
    //   4 = Tagi, 5 = Typ nośnika.
    const int trybGrupowania = ui->comboGrupowanie->currentIndex();

    QMap<int, QString>          slownikKategorii;
    QList<QPair<int, QString>>  listaPlatform;
    QMap<int, QStringList>      mapaTagow;
    QMap<int, QString>          mapaTypowNosnika; // id_platformy -> typ_nosnika (tryb 5)

    if (trybGrupowania == 0) slownikKategorii = appController.pobierzSlownikKategorii();
    if (trybGrupowania == 2) listaPlatform    = appController.pobierzPlatformy();
    if (trybGrupowania == 4) mapaTagow        = appController.pobierzPrzypisaniaTagow();
    if (trybGrupowania == 5) {
        for (const auto& p : appController.pobierzPelnePlatformy())
            mapaTypowNosnika.insert(p.id, p.typNosnika);
    }

    // Mapa istniejących węzłów-folderów, by nie tworzyć duplikatów.
    QMap<QString, QTreeWidgetItem*> wezlyGlowne;

    // Etap 1: Kategoria/Platforma/Tagi mają w bazie własną tabelę-słownik (pełną listę grup),
    // więc foldery tworzymy z góry — dzięki temu widać też grupy puste (bez żadnego medium).
    // Status/Data/Typ nośnika nie mają słownika (wynikają z samych mediów), więc ich foldery
    // powstają dynamicznie w Etapie 2.
    if (trybGrupowania == 0) {
        for (const auto& kat : appController.pobierzKategorie()) {
            QTreeWidgetItem *wezel = new QTreeWidgetItem(ui->drzewoMedow);
            wezel->setText(0, kat.second);
            wezlyGlowne.insert(kat.second, wezel);
        }
    } else if (trybGrupowania == 2) {
        for (const auto& plat : listaPlatform) {
            QTreeWidgetItem *wezel = new QTreeWidgetItem(ui->drzewoMedow);
            wezel->setText(0, plat.second);
            wezlyGlowne.insert(plat.second, wezel);
        }
    } else if (trybGrupowania == 4) {
        // Ładujemy wszystkie tagi z bazy, żeby puste tagi też pojawiły się w drzewie.
        for (const auto& tag : appController.pobierzTagi()) {
            QTreeWidgetItem *wezel = new QTreeWidgetItem(ui->drzewoMedow);
            wezel->setText(0, tag.second);
            wezlyGlowne.insert(tag.second, wezel);
        }
    }

    // Etap 2: Przypisujemy każde medium do odpowiedniego folderu.
    for (const auto& medium : listaMultimediow) {

        // Tagi to relacja wiele-do-wielu — jedno medium może trafić do kilku folderów.
        if (trybGrupowania == 4) {
            QStringList tagiMedium = mapaTagow.value(medium->id);
            if (tagiMedium.isEmpty()) tagiMedium << "Brak tagów";

            for (const QString& nazwaGrupy : tagiMedium) {
                if (!wezlyGlowne.contains(nazwaGrupy)) {
                    QTreeWidgetItem *nowyWezel = new QTreeWidgetItem(ui->drzewoMedow);
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
            QTreeWidgetItem *nowyWezel = new QTreeWidgetItem(ui->drzewoMedow);
            nowyWezel->setText(0, nazwaGrupy);
            wezlyGlowne.insert(nazwaGrupy, nowyWezel);
        }

        QTreeWidgetItem *item = new QTreeWidgetItem(wezlyGlowne[nazwaGrupy]);
        item->setText(0, medium->tytul);
        item->setData(0, Qt::UserRole, medium->id);
    }

    ui->drzewoMedow->expandAll();
}

void NavigationPanelWidget::pokazMenuDrzewa(const QPoint &pos)
{
    // selectedItems() — wszystkie aktualnie zaznaczone węzły (multi-select włączony w konstruktorze).
    const QList<QTreeWidgetItem*> zaznaczone = ui->drzewoMedow->selectedItems();
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

    // bierze miejsce prawego kliknięcia w drzewie, przelicza je na pozycję na ekranie i pokazuje tam menu, czekając na wybór
    menu.exec(ui->drzewoMedow->viewport()->mapToGlobal(pos));
}
