#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QSqlQuery>

#include "multimedia.h"
#include "activity_statistics.h"
#include "history.h"

// DatabaseManager jest jedyną klasą mającą bezpośredni dostęp do bazy danych.
// Cała warstwa SQL jest zamknięta tutaj — reszta aplikacji posługuje się
// wyłącznie modelami (Multimedia, HistoricalAttempt itp.) i typami Qt.
//
// Połączenie czyta konfigurację z config.ini (klucze: database/host,
// database/name, database/user, database/password).
class DatabaseManager {
private:
    QSqlDatabase db;

public:
    DatabaseManager();
    ~DatabaseManager() { zamknijPolaczenie(); }

    // Połączenie
    bool otworzPolaczenie();
    void zamknijPolaczenie();

    // Multimedia
    QList<std::shared_ptr<Multimedia>> pobierzWszystkieMultimedia();
    int  dodajNoweMedium(const QString &tytul, int idKat, int idPlatformy, int cel, int rokWydania, const QString &tworcy);
    bool aktualizujDaneMedium(int id, const QString &tytul, int idKat, int idPlatformy, int cel, int rokWydania, const QString &tworcy);
    bool usunMedium(int id);
    bool usunWieleMultimediow(const QList<int>& listaId);
    bool zmienKategorieWielu(const QList<int>& idMultimediow, int nowaKategoriaId);
    bool ustawUlubione(int idMedium, bool ulubione);

    // Postęp
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
    QMap<int, QString>           pobierzSlownikKategorii();
    QList<QPair<int, QString>>   pobierzKategorie();
    QList<QList<QVariant>>       pobierzSuroweKategorie();
    QMap<int, QString>           pobierzSlownikJednostek();
    QStringList                  pobierzUnikalneJednostki();
    int  dodajKategorie(const QString &nazwa, const QString &jednostka);
    bool aktualizujKategorie(int id, const QString &nazwa, const QString &jednostka);
    // usunPowiazane=true: kasuje multimedia tej kategorii; false: ustawia im id_kategorii = NULL.
    bool usunKategorie(int idKat, bool usunPowiazane);

    // Platformy
    QList<QPair<int, QString>> pobierzPlatformy();
    // Pełne dane platform wraz z typem nośnika: wiersz = [id, nazwa, typ_nosnika].
    QList<QList<QVariant>>     pobierzSurowePlatformy();
    int  dodajPlatforme(const QString &nazwa, const QString &typNosnika);
    bool aktualizujPlatforme(int id, const QString &nazwa, const QString &typNosnika);
    // Analogicznie jak usunKategorie — usunPowiazane kontroluje los multimedia.
    bool usunPlatforme(int idPlat, bool usunPowiazane);

    //Tagi
    QList<QPair<int, QString>>   pobierzTagi();
    QMap<int, QStringList>       pobierzPrzypisaniaTagow();
    int  dodajTag(const QString &nazwa);
    bool aktualizujTag(int id, const QString &nazwa);
    bool usunTag(int idTagu);
    // Pełna wymiana tagów: DELETE wszystkich przypisań + INSERT nowych
    bool ustawTagiDlaMedium(int idMedium, const QList<int>& idTagow);

    // Historia i podgląd
    QList<HistoricalAttempt> pobierzPelnaHistorie(int idMedium);
    // Zwraca zakończone podejścia (z oceną lub recenzją); tytuł medium jest w polu tytulMedium.
    QList<HistoricalAttempt> pobierzWszystkieRecenzje();

    // Statystyki
    QMap<QString, int>           pobierzStatystykiGlobalne();
    QVariantMap                  pobierzStatystykiPodsumowania();
    QList<QVariantMap>           pobierzPodsumowanieKategorii();
    QList<QVariantMap>           pobierzPodsumowanieTagow();
    QVariantMap                  pobierzCiekawostkiStatystyczne();
    // Zwraca dane dla wykresu słupkowego; metryka: "czas" (sekundy), "sesje", "jednostki".
    QList<ActivityStatistic>  pobierzSuroweDaneStatystyk(int zakresDni, const QString& metryka);

    QList<int> pobierzOstatnioAktywne(int limit);
};

#endif
