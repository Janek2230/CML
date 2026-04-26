#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QVariantMap>
#include "databasemanager.h"

class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(QObject *parent = nullptr);

    bool inicjalizujBaze();

    DatabaseManager& getDb() { return dbManager; }

    QList<QVariantMap> pobierzDaneDlaWykresu(int zakres, const QString& metryka);

signals:
    void bladKrytyczny(const QString& wiadomosc);

private:
    DatabaseManager dbManager;
};

#endif