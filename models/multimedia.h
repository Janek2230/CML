#ifndef MULTIMEDIA_H
#define MULTIMEDIA_H

#include <QString>
#include <QDateTime>
#include "postep.h"

class Multimedia {
private:
    int id;
    QString tytul;
    int idKategorii;
    QString status;
    Postep postep;
    QDateTime dataDodania;
    QDateTime dataOstatniejEdycji;
    int ocena;

    int idPlatformy;

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
    QDateTime getDataOstatniejEdycji() const { return dataOstatniejEdycji; }
    void setDataOstatniejEdycji(const QDateTime &d) { dataOstatniejEdycji = d; }

    void setDataDodania(const QDateTime &data) { dataDodania = data; }
    void setPostep(Postep nowyPostep) {postep = nowyPostep;}
    void setStatus(QString nowyStatus) {status = nowyStatus;}
};

#endif