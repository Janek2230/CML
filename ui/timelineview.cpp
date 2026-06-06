#include "timelineview.h"
#include "ui_timelineview.h"
#include <QLocale>
#include <QLabel>
#include <QPushButton>
#include <QDialog>
#include <QTextBrowser>

TimelineView::TimelineView(AppController& controller, QWidget *parent)
    : QWidget(parent), appController(controller), ui(new Ui::TimelineView)
{
    ui->setupUi(this);
    glownyUklad = qobject_cast<QVBoxLayout*>(ui->scrollAreaWidgetContents->layout());
    glownyUklad->setAlignment(Qt::AlignTop);
}

TimelineView::~TimelineView()
{
    delete ui;
}

// Usuwa wszystkie widgety z layoutu przed ponownym renderowaniem timeline.
// takeAt(0) wyciąga elementy jeden po jednym, bez tego renderujOsCzasu() dokładałoby
// karty na koniec zamiast odświeżać listę od zera.
void TimelineView::wyczyscLayout(QLayout* layout) {
    if (!layout) return;
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        // QLayoutItem to opakowanie: trzyma albo widget, albo pod-layout (albo spacer).
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        } else if (QLayout* layoutPotomny = item->layout()) {
            wyczyscLayout(layoutPotomny);   // pod-layout: najpierw rekurencyjnie opróżniamy jego wnętrze
        }
        delete item;   // zawsze: takeAt oddał opakowanie na własność, więc je zwalniamy
    }
}

void TimelineView::renderujOsCzasu() {
    // Czyścimy ekran przed ponownym rysowaniem — żeby nie dokładać kart
    // na już istniejące gdy użytkownik wraca do tej zakładki.
    wyczyscLayout(glownyUklad);
    const auto recenzje = appController.pobierzWszystkieRecenzje();

    // obecnyMiesiacRok śledzi jaki miesiąc był ostatnio narysowany —
    // dzięki temu nagłówki z miesiącami pojawiają się tylko raz.
    QString obecnyMiesiacRok = "";
    QLocale polski(QLocale::Polish, QLocale::Poland);

    // Pusty stan — zabezpieczenie.
    if (recenzje.isEmpty()) {
        QLabel* puste = new QLabel("Nie masz jeszcze żadnych zakończonych tytułów z recenzją.");
        puste->setStyleSheet("color: #8a8f98; font-size: 16px;");
        puste->setAlignment(Qt::AlignCenter);
        glownyUklad->addWidget(puste);
        return;
    }

    for (const auto& p : recenzje) {
        // Budujemy napis "STYCZEŃ 2025" i porównujemy z poprzednim.
        QString nazwaMiesiaca = polski.standaloneMonthName(p.data_rozpoczecia.date().month());
        QString dataStr = QString("%1 %2").arg(nazwaMiesiaca).arg(p.data_rozpoczecia.date().year()).toUpper();

        if (dataStr != obecnyMiesiacRok) {
            obecnyMiesiacRok = dataStr;
            QLabel* naglowek = new QLabel(obecnyMiesiacRok);
            naglowek->setStyleSheet("color: #2d89ef; font-weight: bold; font-size: 20px; margin-top: 15px; border-bottom: 2px solid #333; padding-bottom: 5px;");
            glownyUklad->addWidget(naglowek);
        }

        QString tytulMedium = p.tytulMedium.isEmpty() ? "Nieznany tytuł" : p.tytulMedium;
        QString wlasciwaRecenzja = p.recenzja;

        if (wlasciwaRecenzja.trimmed().isEmpty() || wlasciwaRecenzja.trimmed() == QString::number(p.ocena)) {
            wlasciwaRecenzja = "Brak recenzji tekstowej.";
        }

        // simplified() spłaszcza tekst — zamienia entery i wielokrotne spacje w jedną spację,
        // żeby podgląd był zawsze jedną linią a nie blokiem tekstu.
        QString plaskiTekst = wlasciwaRecenzja.simplified();
        QString podgladTekstu;

        // Podgląd ucinamy do 90 znaków żeby karty miały zbliżoną wysokość.
        // Reszta tekstu dostępna po kliknięciu w dialog.
        if (plaskiTekst.length() > 90) {
            podgladTekstu = plaskiTekst.left(90) + "... <span style='color: #2d89ef; font-style: normal; font-size: 11px;'>(Kliknij, by czytać więcej)</span>";
        } else {
            podgladTekstu = plaskiTekst;
        }

        // Karta to QPushButton a nie zwykły widget — bo przycisk obsługuje kliknięcie
        // i efekt hover bez żadnego dodatkowego kodu.
        QPushButton* karta = new QPushButton();
        karta->setCursor(Qt::PointingHandCursor);
        karta->setMinimumHeight(90);
        karta->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        karta->setStyleSheet(
            "QPushButton {"
            "   text-align: left;"
            "   background: #25282a;"
            "   border-radius: 8px;"
            "   border-left: 5px solid #2d89ef;"
            "}"
            "QPushButton:hover { background: #2f3336; }"
            );

        // Layout wewnątrz przycisku — QPushButton może trzymać własne widgety,
        // dzięki temu karta ma tytuł i fragment zamiast jednego napisu.
        QVBoxLayout* l = new QVBoxLayout(karta);
        l->setContentsMargins(15, 15, 15, 15);
        l->setSpacing(5);

        QLabel* tytul = new QLabel(QString("<span style='font-size: 16px; font-weight: bold; color: white;'>%1</span> <span style='color: #f1c40f; font-weight: bold;'>(Ocena: %2/10)</span>").arg(tytulMedium).arg(p.ocena));

        QLabel* fragment = new QLabel(podgladTekstu);
        fragment->setWordWrap(true);
        fragment->setTextFormat(Qt::RichText); // Bez tego tagi <span> wyświetlają się jako zwykły tekst.
        fragment->setStyleSheet("color: #bdc3c7; font-style: italic;");
        // Bez tego atrybutu kliknięcie w obszar labela zatrzymuje się na nim
        // i nie dociera do przycisku pod spodem.
        fragment->setAttribute(Qt::WA_TransparentForMouseEvents);

        l->addWidget(tytul);
        l->addWidget(fragment);

        // Lambda przechwytuje p przez wartość (kopię) — każda karta musi pamiętać
        // swoje własne dane niezależnie od pozostałych kart w pętli.
        connect(karta, &QPushButton::clicked, this, [this, p]() {
            pokazDetaleRecenzji(p);
        });

        //dokłada gotową kartę na dół listy
        glownyUklad->addWidget(karta);
    }
}

void TimelineView::pokazDetaleRecenzji(const HistoricalAttempt& p) {
    // "this" jako rodzic dialogu — dialog wycentruje się nad oknem aplikacji
    // i zostanie zamknięty razem z nią gdyby coś poszło nie tak.
    QDialog d(this);
    QString tytulMedium = p.tytulMedium.isEmpty() ? "Nieznany tytuł" : p.tytulMedium;
    QString wlasciwaRecenzja = p.recenzja;

    // Ten sam filtr co w renderujOsCzasu: recenzja równa samej ocenie = brak recenzji
    if (wlasciwaRecenzja.trimmed().isEmpty() || wlasciwaRecenzja.trimmed() == QString::number(p.ocena)) {
        wlasciwaRecenzja = "Brak recenzji tekstowej.";
    }

    d.setWindowTitle(QString("Wspomnienia: %1").arg(tytulMedium));
    d.resize(500, 600);
    QVBoxLayout* layout = new QVBoxLayout(&d);

    layout->addWidget(new QLabel(QString("<h2>Werdykt: %1/10</h2>").arg(p.ocena)));

    // QTextBrowser zamiast QLabel — recenzja może być długa i wieloliniowa,
    // QTextBrowser ma własny scrollbar.
    QTextBrowser* rec = new QTextBrowser();
    rec->setHtml(QString("<i>%1</i>").arg(wlasciwaRecenzja));
    rec->setStyleSheet("background: #1a1c1e; padding: 10px; border-radius: 5px; color: #d1d1d1; border-left: 3px solid #f1c40f;");
    rec->setMaximumHeight(150);
    layout->addWidget(rec);

    // Osobna scroll area na sesje — niezależna od recenzji
    layout->addWidget(new QLabel("<br><b>Oś czasu sesji (Notatki):</b>"));
    QScrollArea* sa = new QScrollArea();
    QWidget* zawartosc = new QWidget();
    QVBoxLayout* layoutNotatek = new QVBoxLayout(zawartosc);

    if (p.sesje.isEmpty()) {
        layoutNotatek->addWidget(new QLabel("Brak notatek z przebiegu sesji."));
    } else {
        for (const auto& s : p.sesje) {
            QLabel* n = new QLabel(QString("<span style='color:#2d89ef; font-weight:bold;'>%1</span><br>%2")
                                       .arg(s.data.toString("dd.MM.yyyy HH:mm"), s.notatka.isEmpty() ? "Brak notatki" : s.notatka));
            n->setWordWrap(true);
            n->setStyleSheet("margin-bottom: 5px; padding: 10px; background: #25282a; border-radius: 5px;");
            layoutNotatek->addWidget(n);
        }
    }
    // addStretch wypycha wszystkie wpisy do góry zamiast rozciągać je na całą dostępną wysokość.
    layoutNotatek->addStretch();

    sa->setWidget(zawartosc);
    sa->setWidgetResizable(true);
    layout->addWidget(sa);

    // exec() blokuje — kod poniżej nie wykona się dopóki użytkownik nie zamknie dialogu.
    d.exec();
}
