#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QSqlQuery>

#include "multimedia.h"

class DatabaseManager {
private:
    QSqlDatabase db;
public:
    DatabaseManager();
    ~DatabaseManager() { closeConnection(); }

    bool openConnection();
    void closeConnection();

    QMap<int, QString> getCategories();
    QList<std::shared_ptr<Multimedia>> getAllMultimedia();
    QMap<QString, int> getGlobalStats();

    bool aktualizujPostep(int idMedium, const QString& status, int aktualna, int docelowa);
    bool zacznijOdNowa(int idMedium);

    bool dodajNoweMedium(const QString &tytul, int idKat, const QString &typ, int cel);
    bool usunMedium(int id);

    QList<QPair<int, QString>> pobierzKategorie();
    QStringList pobierzPlatformy();
};

#endif