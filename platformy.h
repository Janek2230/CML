#ifndef PLATFORMY_H
#define PLATFORMY_H

#include <QString>

class Platformy {
private:
    int id;
    QString nazwa;
    QString typNosnika;

public:
    Platformy(int id, QString nazwa, QString typ)
        : id(id), nazwa(nazwa), typNosnika(typ) {}

    int getId() const { return id; }
    QString getNazwa() const { return nazwa; }
    QString getTyp() const { return typNosnika; }
};

#endif