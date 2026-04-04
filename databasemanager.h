#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QSqlQuery>

#include "postep_ksiazki.h"

class DatabaseManager {
private:
    QSqlDatabase db;

public:
    DatabaseManager();
    ~DatabaseManager() {closeConnection();}

    bool openConnection();
    void closeConnection();

    QList<Ksiazka*> getBooks();
};

#endif