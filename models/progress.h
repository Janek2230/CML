#ifndef PROGRESS_H
#define PROGRESS_H

#include <QString>

// Stan bieżącego podejścia do medium — osadzony bezpośrednio w obiekcie Multimedia,
// żeby widoki nie musiały osobno odpytywać tabeli podejść.
struct Progress {
    int aktualna = 0;
    int docelowa = 0;
    QString jednostka;
    int numer_podejscia = 0;

    double pobierzProcent() const {
        return (docelowa > 0) ? (static_cast<double>(aktualna) / docelowa * 100.0) : 0.0;
    }
};

#endif