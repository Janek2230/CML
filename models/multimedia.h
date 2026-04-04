#ifndef MULTIMEDIA_H
#define MULTIMEDIA_H

#include <QString>

class Multimedia {
protected:
    int id;
    QString tytul;
    QString typMedium;
    QString status;
    int idPlatformy;

public:
    Multimedia(int id, QString tytul, QString typ, QString status, int platforma)
        : id(id), tytul(tytul), typMedium(typ), status(status), idPlatformy(platforma){}
    virtual ~Multimedia() {}

    int getId() const { return id; }
    QString getTytul() const { return tytul; }
    QString getTyp() const { return typMedium; }
    QString getStatus() const { return status; }
    int getIdPlatformy() const { return idPlatformy; }
};

#endif // MULTIMEDIA_H