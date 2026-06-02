#ifndef MULTIMEDIA_H
#define MULTIMEDIA_H

#include <QString>
#include <QDateTime>
#include "postep.h"

// Pojedyncze medium w bibliotece — książka, gra, serial, film itp.
// Obiekt jest tworzony przez getAllMultimedia() i przekazywany jako shared_ptr.
// Pola uzupełniane setterami (dataDodania, ocena, ulubione...) są opcjonalne
// i ustawiane zaraz po konstrukcji na podstawie kolejnych kolumn zapytania SQL.
class Multimedia {
private:
    int id;
    QString tytul;
    int idKategorii;
    QString status;
    Postep postep;
    QDateTime dataDodania;
    QDateTime dataOstatniejAktywnosci;
    int ocena = 0; // Inicjalizacja chroni przed UB gdy setOcena() nie zostało wywołane.

    int idPlatformy;
    bool ulubione = false;
    int rokWydania = 0;   // 0 == nieznany; ustawiane setterem na podstawie kolumny rok_wydania.
    QString tworcy;       // Autor / reżyser / studio; może być puste.

public:
    Multimedia(int id, QString tytul, int idKat, int idPlat, QString status, Postep postep)
        : id(id), tytul(tytul), idKategorii(idKat), idPlatformy(idPlat), status(status), postep(postep) {}

    int getId() const { return id; }
    QString getTytul() const { return tytul; }
    int getIdKategorii() const { return idKategorii; }
    QString getStatus() const { return status; }
    Postep getPostep() const { return postep; }
    QDateTime getDataDodania() const { return dataDodania; }
    int getIdPlatformy() const { return idPlatformy; }

    int getOcena() const { return ocena; }
    void setOcena(int o) { ocena = o; }
    bool getCzyUlubione() const { return ulubione; }
    void setCzyUlubione(bool val) { ulubione = val; }
    QDateTime getDataOstatniejAktywnosci() const { return dataOstatniejAktywnosci; }
    void setDataOstatniejAktywnosci(const QDateTime &d) { dataOstatniejAktywnosci = d; }

    int getRokWydania() const { return rokWydania; }
    void setRokWydania(int rok) { rokWydania = rok; }
    QString getTworcy() const { return tworcy; }
    void setTworcy(const QString &t) { tworcy = t; }

    void setDataDodania(const QDateTime &data) { dataDodania = data; }
    void setPostep(Postep nowyPostep) { postep = nowyPostep; }
    void setStatus(QString nowyStatus) { status = nowyStatus; }
};

#endif