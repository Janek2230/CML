#ifndef POSTEP_H
#define POSTEP_H

#include <QString>

// Stan bieżącego podejścia do medium — osadzony bezpośrednio w obiekcie Multimedia,
// żeby widoki nie musiały osobno odpytywać tabeli podejść.
struct Postep {
    int aktualna;
    int docelowa;
    QString jednostka;
    int numer_podejscia;

    double getProcent() const {
        return (docelowa > 0) ? (static_cast<double>(aktualna) / docelowa * 100.0) : 0.0;
    }
};

#endif