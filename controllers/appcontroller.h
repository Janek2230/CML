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
    QVariantMap pobierzStatystykiPodsumowania();
    QList<QVariantMap> pobierzPodsumowanieKategorii();
    QList<QVariantMap> pobierzPodsumowanieTagow();
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
    int dodajTag(const QString &nazwa);
    bool aktualizujPlatforme(int id, const QString &nazwa);
    bool aktualizujTag(int id, const QString &nazwa);
    bool usunTag(int idTagu);
    QStringList pobierzUnikalneJednostki();
    QList<QPair<int, QString>> pobierzKategorie();
    QList<QPair<int, QString>> pobierzPlatformy();
    QList<QPair<int, QString>> pobierzTagi();
    QMap<int, QString> pobierzSlownikJednostek();
    QList<PodejscieHistoryczne> pobierzHistorie(int idMedium);
    bool dodajPodejscie(int idMedium, const QString& status, int docelowa);
    bool aktualizujPodejscie(int idPodejscia, const QString& status, int aktualna, int docelowa, int ocena, const QString& recenzja);
    bool usunPodejscie(int idPodejscia);
    bool dodajSesje(int idPodejscia, int przyrost, int sekundy, const QString& notatka);
    bool aktualizujSesje(int idSesji, int przyrost, int sekundy, const QString& notatka);
    bool usunSesje(int idSesji);
    bool ustawUlubione(int idMedium, bool ulubione);

    QVariantMap pobierzCiekawostkiStatystyczne();


signals:
    void bladKrytyczny(const QString& wiadomosc);
    void daneZmienione();

private:
    DatabaseManager dbManager;
};

#endif
