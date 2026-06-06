#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QVariantMap>
#include "databasemanager.h"

// AppController jest warstwą pośrednią między widżetami UI a DatabaseManager.
// emituje daneZmienione(), żeby drzewo nawigacji i dashboard automatycznie się odświeżyły.
// Widżety nigdy nie wywołują DatabaseManager bezpośrednio.
class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(QObject *parent = nullptr);

    bool inicjalizujBaze();

    // Multimedia
    QList<std::shared_ptr<Multimedia>> pobierzWszystkieMultimedia();
    int  dodajNoweMedium(const QString &tytul, int idKat, int idPlatformy, int cel, int rokWydania = 0, const QString &tworcy = QString());
    bool aktualizujDaneMedium(int id, const QString &tytul, int idKat, int idPlatformy, int cel, int rokWydania = 0, const QString &tworcy = QString());
    bool usunMedium(int idMedium);
    bool usunWieleMultimediow(const QList<int>& listaId);
    bool zmienKategorieWielu(const QList<int>& idMultimediow, int nowaKategoriaId);
    bool ustawUlubione(int idMedium, bool ulubione);
    QList<int> pobierzOstatnioAktywne(int limit);

    // Postęp
    bool czyOsiagnietoCel(int aktualna, int docelowa);
    bool aktualizujPostep(int idMedium, const QString& status, int aktualna, int docelowa, int ocena);

    // Podejścia
    bool dodajPodejscie(int idMedium, const QString& status, int docelowa);
    bool aktualizujPodejscie(int idPodejscia, const QString& status, int aktualna, int docelowa, int ocena, const QString& recenzja);
    bool usunPodejscie(int idPodejscia);

    // Sesje
    bool dodajSesje(int idPodejscia, int przyrost, int sekundy, const QString& notatka);
    bool aktualizujSesje(int idSesji, int przyrost, int sekundy, const QString& notatka);
    bool usunSesje(int idSesji);

    // Kategorie
    struct KategoriaModel { int id; QString nazwa; QString jednostka; };
    QMap<int, QString>         pobierzSlownikKategorii();
    QList<QPair<int, QString>> pobierzKategorie();
    QList<KategoriaModel>      pobierzPelneKategorie();
    QStringList                pobierzUnikalneJednostki();
    QMap<int, QString>         pobierzSlownikJednostek();
    bool dodajKategorie(const QString &nazwa, const QString &jednostka);
    bool aktualizujKategorie(int id, const QString &nazwa, const QString &jednostka);
    bool usunKategorie(int idKat, bool usunPowiazane);

    // Platformy
    struct PlatformaModel { int id; QString nazwa; QString typNosnika; };
    QList<QPair<int, QString>> pobierzPlatformy();
    QList<PlatformaModel>      pobierzPelnePlatformy();
    int  dodajPlatforme(const QString &nazwa, const QString &typNosnika = QString());
    bool aktualizujPlatforme(int id, const QString &nazwa, const QString &typNosnika = QString());
    bool usunPlatforme(int idPlat, bool usunPowiazane);

    // Tagi
    QList<QPair<int, QString>> pobierzTagi();
    QMap<int, QStringList>     pobierzPrzypisaniaTagow();
    int  dodajTag(const QString &nazwa);
    bool aktualizujTag(int id, const QString &nazwa);
    bool usunTag(int idTagu);
    bool ustawTagiDlaMedium(int idMedium, const QList<int>& idTagow);

    // Historia i podgląd
    QList<HistoricalAttempt> pobierzHistorie(int idMedium);
    QList<HistoricalAttempt> pobierzWszystkieRecenzje();

    // Statystyki
    QMap<QString, int>  pobierzStatystykiGlobalne();
    QVariantMap         pobierzStatystykiPodsumowania();
    QList<QVariantMap>  pobierzPodsumowanieKategorii();
    QList<QVariantMap>  pobierzPodsumowanieTagow();
    QVariantMap         pobierzCiekawostkiStatystyczne();
    QList<ActivityStatistic>  pobierzDaneDlaWykresu(int zakres, const QString& metryka);

signals:
    // Emitowany po każdej zmianie danych — wyzwala odświeżenie drzewa i dashboardu.
    void daneZmienione();

private:
    DatabaseManager menedzerBazy;
};

#endif
