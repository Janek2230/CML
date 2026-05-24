#ifndef HISTORIA_H
#define HISTORIA_H

#include <QString>
#include <QDateTime>
#include <QList>

// Pojedynczy wpis dziennika — jeden odcinek czytania/oglądania/grania.
struct Sesja {
    int id;
    QDateTime data;
    int przyrost; // przyrost wartości_aktualnej w tej sesji (w jednostkach kategorii)
    int sekundy;  // zmierzony lub szacowany czas trwania sesji
    QString notatka;
};

// Jedno podejście do medium — od kliknięcia "zacznij" do zakończenia lub porzucenia.
struct PodejscieHistoryczne {
    int id;
    int numer;
    QString status;
    int aktualna;
    int docelowa;
    int ocena;
    QString tytulMedium;
    QString recenzja;
    QDateTime data_rozpoczecia;
    QList<Sesja> sesje;
};

#endif // HISTORIA_H
