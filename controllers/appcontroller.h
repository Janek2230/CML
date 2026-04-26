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

    QList<QVariantMap> pobierzDaneDlaWykresu(int zakres, const QString& metryka);
    QList<std::shared_ptr<Multimedia>> pobierzWszystkieMultimedia();
    QMap<QString, int> getGlobalStats();
    QList<int> pobierzOstatnioAktywne(int limit);

    bool czyOsiagnietoCel(int aktualna, int docelowa);
    bool aktualizujPostep(int idMedium, const QString& status, int aktualna, int docelowa, int ocena);

signals:
    void bladKrytyczny(const QString& wiadomosc);

private:
    DatabaseManager dbManager;
};

#endif
