#ifndef MULTIMEDIA_H
#define MULTIMEDIA_H

#include <QString>
#include "postep.h"

class Multimedia {
private:
    int id;
    QString tytul;
    int idKategorii;
    QString status;
    Postep postep;

public:
    Multimedia(int id, QString tytul, int idKat, QString status, Postep postep)
        : id(id), tytul(tytul), idKategorii(idKat), status(status), postep(postep) {}

    int getId() const { return id; }
    QString getTytul() const { return tytul; }
    int getIdKategorii() const { return idKategorii; }
    QString getStatus() const { return status; }
    Postep getPostep() const { return postep; }

    void setPostep(Postep nowyPostep) {postep = nowyPostep;}
    void setStatus(QString nowyStatus) {status = nowyStatus;}
};

#endif