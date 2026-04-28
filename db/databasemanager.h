#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QSqlQuery>

#include "multimedia.h"
#include "aktywnosc_statystyki.h"

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

    bool aktualizujPostep(int idMedium, const QString& status, int aktualna, int docelowa, int ocena);
    bool zacznijOdNowa(int idMedium);

    bool dodajNoweMedium(const QString &tytul, int idKat, int idPlatformy, int cel);
    bool aktualizujDaneMedium(int id, const QString &tytul, int idKat, int idPlatformy, int cel);
    bool usunMedium(int id);

    int dodajKategorie(const QString &nazwa, const QString &jednostka);
    int dodajPlatforme(const QString &nazwa);
    QStringList pobierzUnikalneJednostki();

    QList<QPair<int, QString>> pobierzKategorie();
    QList<QPair<int, QString>> pobierzPlatformy();
    QList<QList<QVariant>> pobierzSuroweKategorie();

    bool aktualizujKategorie(int id, const QString &nazwa, const QString &jednostka);
    bool aktualizujPlatforme(int id, const QString &nazwa);

    bool usunKategorie(int idKat, bool usunPowiazane);
    bool usunPlatforme(int idPlat, bool usunPowiazane);

    bool zmienKategorieWielu(const QList<int>& idMultimediow, int nowaKategoriaId);
    bool usunWieleMultimediow(const QList<int>& idList);

    QMap<int, QString> pobierzSlownikJednostek();
    QList<StatystykaAktywnosci> pobierzSuroweDaneStatystyk(int zakresDni, const QString& metryka);
    QList<int> pobierzOstatnioAktywne(int limit);
    QList<PodejscieHistoryczne> pobierzPelnaHistorie(int idMedium);
};

#endif
