#ifndef POSTEP_H
#define POSTEP_H

#include <QString>
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