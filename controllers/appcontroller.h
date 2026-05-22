#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QVariantMap>
#include "databasemanager.h"

// AppController jest cienką warstwą pośrednią między widżetami UI a DatabaseManager.
// Jego jedyna odpowiedzialność biznesowa: po każdej mutacji danych emitować daneZmienione(),
// żeby drzewo nawigacji i dashboard automatycznie się odświeżyły.
// Widżety nigdy nie wywołują DatabaseManager bezpośrednio.
class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(QObject *parent = nullptr);

    bool inicjalizujBaze();

    // --- Multimedia ---
    QList<std::shared_ptr<Multimedia>> pobierzWszystkieMultimedia();
    QList<std::shared_ptr<Multimedia>> pobierzKupkeWstydu();
    int  dodajNoweMedium(const QString &tytul, int idKat, int idPlatformy, int cel);
    bool aktualizujDaneMedium(int id, const QString &tytul, int idKat, int idPlatformy, int cel);
    bool usunMedium(int idMedium);
    bool usunWieleMultimediow(const QList<int>& idList);
    bool zmienKategorieWielu(const QList<int>& idMultimediow, int nowaKategoriaId);
    bool ustawUlubione(int idMedium, bool ulubione);
    QList<int> pobierzOstatnioAktywne(int limit);

    // --- Postęp ---
    bool czyOsiagnietoCel(int aktualna, int docelowa);
    // aktualizujPostep celowo NIE emituje daneZmienione — patrz implementacja.
    bool aktualizujPostep(int idMedium, const QString& status, int aktualna, int docelowa, int ocena);
    bool zacznijOdNowa(int idMedium);

    // --- Podejścia ---
    bool dodajPodejscie(int idMedium, const QString& status, int docelowa);
    bool aktualizujPodejscie(int idPodejscia, const QString& status, int aktualna, int docelowa, int ocena, const QString& recenzja);
    bool usunPodejscie(int idPodejscia);

    // --- Sesje ---
    bool dodajSesje(int idPodejscia, int przyrost, int sekundy, const QString& notatka);
    bool aktualizujSesje(int idSesji, int przyrost, int sekundy, const QString& notatka);
    bool usunSesje(int idSesji);

    // --- Kategorie ---
    struct KategoriaModel { int id; QString nazwa; QString jednostka; };
    QMap<int, QString>         getCategories();
    QList<QPair<int, QString>> pobierzKategorie();
    QList<KategoriaModel>      pobierzPelneKategorie();
    QStringList                pobierzUnikalneJednostki();
    QMap<int, QString>         pobierzSlownikJednostek();
    QStringList                pobierzDostepneStatusy();
    int  dodajKategorie(const QString &nazwa, const QString &jednostka);
    bool aktualizujKategorie(int id, const QString &nazwa, const QString &jednostka);
    bool usunKategorie(int idKat, bool usunPowiazane);

    // --- Platformy ---
    QList<QPair<int, QString>> pobierzPlatformy();
    int  dodajPlatforme(const QString &nazwa);
    bool aktualizujPlatforme(int id, const QString &nazwa);
    bool usunPlatforme(int idPlat, bool usunPowiazane);

    // --- Tagi ---
    QList<QPair<int, QString>> pobierzTagi();
    QMap<int, QStringList>     pobierzPrzypisaniaTagow();
    int  dodajTag(const QString &nazwa);
    bool aktualizujTag(int id, const QString &nazwa);
    bool usunTag(int idTagu);
    bool ustawTagiDlaMedium(int idMedium, const QList<int>& idTagow);

    // --- Historia i podgląd ---
    QList<PodejscieHistoryczne> pobierzHistorie(int idMedium);
    QList<PodejscieHistoryczne> pobierzWszystkieRecenzje();

    // --- Statystyki ---
    QMap<QString, int>  getGlobalStats();
    QVariantMap         pobierzStatystykiPodsumowania();
    QList<QVariantMap>  pobierzPodsumowanieKategorii();
    QList<QVariantMap>  pobierzPodsumowanieTagow();
    QVariantMap         pobierzCiekawostkiStatystyczne();
    QList<QVariantMap>  pobierzDaneDlaWykresu(int zakres, const QString& metryka);

signals:
    void bladKrytyczny(const QString& wiadomosc);
    // Emitowany po każdej mutacji danych — wyzwala odświeżenie drzewa i dashboardu.
    void daneZmienione();

private:
    DatabaseManager dbManager;
};

#endif
