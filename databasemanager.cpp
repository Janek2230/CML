#include "databasemanager.h"

DatabaseManager::DatabaseManager() {
    db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("localhost");
    db.setDatabaseName("cml");
    db.setUserName("postgres");
    db.setPassword("Q@wertyuiop");
}

bool DatabaseManager::openConnection() {
    if (!db.open()) {
        qDebug() << "Błąd połączenia z bazą:" << db.lastError().text();
        return false;
    }
    qDebug() << "Połączono z bazą PostgreSQL!";
    return true;
}

void DatabaseManager::closeConnection() {
    db.close();
}

QList<Ksiazka*> DatabaseManager::getBooks() {
    QList<Ksiazka*> list;

    QSqlQuery query("SELECT m.id, m.tytul, m.typ_medium, m.status, m.id_platformy, "
                    "p.przeczytane_strony, p.wszystkie_strony "
                    "FROM multimedia m "
                    "JOIN postepy_ksiazki p ON m.id = p.id_medium");

    while (query.next()) {

        list.append(new Ksiazka(
            query.value(0).toInt(),     // id
            query.value(1).toString(),  // tytul
            query.value(2).toString(),  // typ
            query.value(3).toString(),  // status
            query.value(4).toInt(),     // platforma
            query.value(5).toInt(),     // przeczytane
            query.value(6).toInt()      // wszystkie
            ));
    }
    return list;
}