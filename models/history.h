#ifndef HISTORY_H
#define HISTORY_H

#include <QString>
#include <QDateTime>
#include <QList>

// Pojedynczy wpis dziennika — jeden odcinek czytania/oglądania/grania.
struct Session {
    int id = 0;
    QDateTime data;
    int przyrost = 0; // przyrost wartości_aktualnej w tej sesji (w jednostkach kategorii)
    int sekundy = 0;  // zmierzony lub szacowany czas trwania sesji
    QString notatka;
};

// Jedno podejście do medium — od kliknięcia "zacznij" do zakończenia lub porzucenia.
struct HistoricalAttempt {
    int id = 0;
    int numer = 0;
    QString status;            // "W trakcie" / "Ukończone" / "Porzucone"
    int aktualna = 0;
    int docelowa = 0;
    int ocena = 0;             // skala 0–10; 0 == brak oceny
    QString tytulMedium;
    QString recenzja;
    QDateTime data_rozpoczecia;
    QList<Session> sesje;
};

#endif // HISTORY_H
