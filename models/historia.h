#ifndef HISTORIA_H
#define HISTORIA_H

#include <QString>
#include <QDateTime>

struct Sesja {
    int id;
    QDateTime data;
    int przyrost;
    int sekundy;
    QString notatka;
};

struct PodejscieHistoryczne {
    int id;
    int numer;
    QString status;
    int aktualna;
    int docelowa;
    int ocena;
    QString recenzja;
    QList<Sesja> sesje;
};


#endif // HISTORIA_H
