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

    void setDataDodania(const QDateTime &data) { dataDodania = data; }
    void setPostep(Postep nowyPostep) {postep = nowyPostep;}
    void setStatus(QString nowyStatus) {status = nowyStatus;}
};

#endif