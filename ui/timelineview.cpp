#include "timelineview.h"
#include <QLocale>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QDialog>
#include <QTextBrowser>

TimelineView::TimelineView(AppController& controller, QWidget *parent)
    : QWidget(parent), appController(controller)
{
    QVBoxLayout* baseLayout = new QVBoxLayout(this);
    baseLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; background-color: transparent; }");

    QWidget* scrollContent = new QWidget(scrollArea);
    mainLayout = new QVBoxLayout(scrollContent);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    scrollArea->setWidget(scrollContent);
    baseLayout->addWidget(scrollArea);
}

void TimelineView::wyczyscLayout(QLayout* layout) {
    if (!layout) return;
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        } else if (QLayout* childLayout = item->layout()) {
            wyczyscLayout(childLayout);
        }
        delete item;
    }
}

void TimelineView::renderujTimeline() {
    wyczyscLayout(mainLayout);
    auto recenzje = appController.pobierzWszystkieRecenzje();

    QString obecnyMiesiacRok = "";
    QLocale polski(QLocale::Polish, QLocale::Poland);

    if (recenzje.isEmpty()) {
        QLabel* puste = new QLabel("Nie masz jeszcze żadnych zakończonych tytułów z recenzją.");
        puste->setStyleSheet("color: #8a8f98; font-size: 16px;");
        puste->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(puste);
        return;
    }

    for (const auto& p : recenzje) {
        QString nazwaMiesiaca = polski.standaloneMonthName(p.data_rozpoczecia.date().month());
        QString dataStr = QString("%1 %2").arg(nazwaMiesiaca).arg(p.data_rozpoczecia.date().year()).toUpper();

        if (dataStr != obecnyMiesiacRok) {
            obecnyMiesiacRok = dataStr;
            QLabel* naglowek = new QLabel(obecnyMiesiacRok);
            naglowek->setStyleSheet("color: #2d89ef; font-weight: bold; font-size: 20px; margin-top: 15px; border-bottom: 2px solid #333; padding-bottom: 5px;");
            mainLayout->addWidget(naglowek);
        }

        // Zabezpieczone parsowanie haka "|||"
        QStringList daneRecenzji = p.recenzja.split("|||");
        QString tytulMedium = (daneRecenzji.size() > 1) ? daneRecenzji.first() : "Nieznany tytuł";
        QString wlasciwaRecenzja = (daneRecenzji.size() > 1) ? daneRecenzji.last() : p.recenzja;

        // Tłumienie "0" jeśli recenzja tekstowa jest pusta
        if (wlasciwaRecenzja.trimmed().isEmpty() || wlasciwaRecenzja.trimmed() == QString::number(p.ocena)) {
            wlasciwaRecenzja = "Brak recenzji tekstowej.";
        }

        // simplified() spłaszcza tekst — zamienia entery i wielokrotne spacje w jedną spację.
        QString plaskiTekst = wlasciwaRecenzja.simplified();
        QString podgladTekstu;

        if (plaskiTekst.length() > 90) {
            podgladTekstu = plaskiTekst.left(90) + "... <span style='color: #2d89ef; font-style: normal; font-size: 11px;'>(Kliknij, by czytać więcej)</span>";
        } else {
            podgladTekstu = plaskiTekst;
        }

        QPushButton* karta = new QPushButton();
        karta->setCursor(Qt::PointingHandCursor);
        karta->setMinimumHeight(90); // Wymuszona minimalna wysokość — bez tego layout zgniata kafelki.
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

        QVBoxLayout* l = new QVBoxLayout(karta);
        l->setContentsMargins(15, 15, 15, 15);
        l->setSpacing(5);

        QLabel* tytul = new QLabel(QString("<span style='font-size: 16px; font-weight: bold; color: white;'>%1</span> <span style='color: #f1c40f; font-weight: bold;'>(Ocena: %2/10)</span>").arg(tytulMedium).arg(p.ocena));

        QLabel* snippet = new QLabel(podgladTekstu);
        snippet->setWordWrap(true);
        snippet->setTextFormat(Qt::RichText); // Wymagane, aby zadziałały tagi <span>
        snippet->setStyleSheet("color: #bdc3c7; font-style: italic;");
        snippet->setAttribute(Qt::WA_TransparentForMouseEvents); // Żeby kliknięcie w tekst przenosiło na przycisk

        l->addWidget(tytul);
        l->addWidget(snippet);

        connect(karta, &QPushButton::clicked, this, [this, p]() {
            pokazDetaleRecenzji(p);
        });

        mainLayout->addWidget(karta);
    }
}

void TimelineView::pokazDetaleRecenzji(const PodejscieHistoryczne& p) {
    QDialog d(this);
    QStringList daneRecenzji = p.recenzja.split("|||");
    QString tytulMedium = (daneRecenzji.size() > 1) ? daneRecenzji.first() : "Nieznany tytuł";
    QString wlasciwaRecenzja = (daneRecenzji.size() > 1) ? daneRecenzji.last() : p.recenzja;

    if (wlasciwaRecenzja.trimmed().isEmpty() || wlasciwaRecenzja.trimmed() == "0") {
        wlasciwaRecenzja = "Brak recenzji tekstowej.";
    }

    d.setWindowTitle(QString("Wspomnienia: %1").arg(tytulMedium));
    d.resize(500, 600);
    QVBoxLayout* layout = new QVBoxLayout(&d);

    layout->addWidget(new QLabel(QString("<h2>Werdykt: %1/10</h2>").arg(p.ocena)));

    QTextBrowser* rec = new QTextBrowser();
    rec->setHtml(QString("<i>%1</i>").arg(wlasciwaRecenzja));
    rec->setStyleSheet("background: #1a1c1e; padding: 10px; border-radius: 5px; color: #d1d1d1; border-left: 3px solid #f1c40f;");
    rec->setMaximumHeight(150);
    layout->addWidget(rec);

    layout->addWidget(new QLabel("<br><b>Oś czasu sesji (Notatki):</b>"));
    QScrollArea* sa = new QScrollArea();
    QWidget* content = new QWidget();
    QVBoxLayout* notesLayout = new QVBoxLayout(content);

    if (p.sesje.isEmpty()) {
        notesLayout->addWidget(new QLabel("Brak notatek z przebiegu sesji."));
    } else {
        for (const auto& s : p.sesje) {
            QLabel* n = new QLabel(QString("<span style='color:#2d89ef; font-weight:bold;'>%1</span><br>%2")
                                       .arg(s.data.toString("dd.MM.yyyy HH:mm")).arg(s.notatka.isEmpty() ? "Brak notatki" : s.notatka));
            n->setWordWrap(true);
            n->setStyleSheet("margin-bottom: 5px; padding: 10px; background: #25282a; border-radius: 5px;");
            notesLayout->addWidget(n);
        }
    }
    notesLayout->addStretch();

    sa->setWidget(content);
    sa->setWidgetResizable(true);
    layout->addWidget(sa);

    d.exec();
}
