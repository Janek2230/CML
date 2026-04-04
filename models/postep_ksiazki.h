#ifndef KSIAZKA_H
#define KSIAZKA_H

#include "multimedia.h"

class Ksiazka : public Multimedia {
private:
    int przeczytaneStrony;
    int wszystkieStrony;

public:
    Ksiazka(int id, QString tytul, QString typ, QString status, int idPlatformy,
            int przeczytane, int wszystkie)
        : Multimedia(id, tytul, typ, status, idPlatformy),
        przeczytaneStrony(przeczytane), wszystkieStrony(wszystkie) {}

    int getPrzeczytane() const { return przeczytaneStrony; }
    int getWszystkie() const { return wszystkieStrony; }

    double getProcentPostepu() const {
        return (wszystkieStrony > 0) ? (static_cast<double>(przeczytaneStrony) / wszystkieStrony * 100.0) : 0;
    }
};

#endif