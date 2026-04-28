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
    QStringList pobierzDostepneStatusy();
    QMap<int, QString> getCategories();
    QList<std::shared_ptr<Multimedia>> pobierzKupkeWstydu();

    bool czyOsiagnietoCel(int aktualna, int docelowa);
    bool aktualizujPostep(int idMedium, const QString& status, int aktualna, int docelowa, int ocena);
    bool dodajKategorie(const QString &nazwa, const QString &jednostka);
    bool aktualizujKategorie(int id, const QString &nazwa, const QString &jednostka);
    bool usunKategorie(int idKat, bool usunPowiazane);
    bool usunPlatforme(int idPlat, bool usunPowiazane);
    bool zmienKategorieWielu(const QList<int>& idMultimediow, int nowaKategoriaId);
    bool usunWieleMultimediow(const QList<int>& idList);
    struct KategoriaModel { int id; QString nazwa; QString jednostka; };
    QList<KategoriaModel> pobierzPelneKategorie();

    bool zacznijOdNowa(int idMedium);
    bool dodajNoweMedium(const QString &tytul, int idKat, int idPlatformy, int cel);
    bool aktualizujDaneMedium(int id, const QString &tytul, int idKat, int idPlatformy, int cel);
    bool usunMedium(int idMedium);
    int dodajPlatforme(const QString &nazwa);
    bool aktualizujPlatforme(int id, const QString &nazwa);
    QStringList pobierzUnikalneJednostki();
    QList<QPair<int, QString>> pobierzKategorie();
    QList<QPair<int, QString>> pobierzPlatformy();
    QMap<int, QString> pobierzSlownikJednostek();

signals:
    void bladKrytyczny(const QString& wiadomosc);
    void daneZmienione();

private:
    DatabaseManager dbManager;
};

#endif
