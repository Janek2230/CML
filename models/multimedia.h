#ifndef MULTIMEDIA_H
#define MULTIMEDIA_H

#include <QString>
#include <QStringList>
#include <QDateTime>
#include "progress.h"

// Dozwolone statusy medium / podejścia
namespace Status {
    inline const QString Planowane  = QStringLiteral("Planowane");
    inline const QString WTrakcie   = QStringLiteral("W trakcie");
    inline const QString Wstrzymane = QStringLiteral("Wstrzymane");
    inline const QString Ukonczone  = QStringLiteral("Ukończone");
    inline const QString Porzucone  = QStringLiteral("Porzucone");

    inline QStringList edytowalne() { return { Planowane, WTrakcie, Wstrzymane }; }
    inline QStringList terminalne() { return { Ukonczone, Porzucone }; }
    inline QStringList wszystkie()  { return { Planowane, WTrakcie, Wstrzymane, Ukonczone, Porzucone }; }
}

// Pojedyncze medium w bibliotece — książka, gra, serial, film itp.
// Pola opcjonalne (dataDodania, ocena, ulubione...) wypełnia się zaraz po konstrukcji
// na podstawie kolejnych kolumn zapytania SQL
struct Multimedia {
    int id = 0;
    QString tytul;
    int idKategorii = 0;
    QString status;
    Progress postep;
    QDateTime dataDodania;
    QDateTime dataOstatniejAktywnosci;
    int ocena = 0;

    int idPlatformy = 0;
    bool ulubione = false;
    int rokWydania = 0;   // 0 == nieznany, z kolumny rok_wydania.
    QString tworcy;       // Autor / reżyser / studio, może być puste.
};

#endif