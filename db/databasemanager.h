#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QSqlQuery>

#include "multimedia.h"
#include "aktywnosc_statystyki.h"
#include "historia.h"

// DatabaseManager jest jedyną klasą mającą bezpośredni dostęp do bazy danych.
// Cała warstwa SQL jest zamknięta tutaj — reszta aplikacji posługuje się
// wyłącznie modelami (Multimedia, PodejscieHistoryczne itp.) i typami Qt.
//
// Połączenie czyta konfigurację z config.ini (klucze: database/host,
// database/name, database/user, database/password).
class DatabaseManager {
private:
    QSqlDatabase db;

public:
    DatabaseManager();
    ~DatabaseManager() { closeConnection(); }

    // --- Połączenie ---
    bool openConnection();
    void closeConnection();

    // --- Multimedia (CRUD) ---
    QList<std::shared_ptr<Multimedia>> getAllMultimedia();
    int  dodajNoweMedium(const QString &tytul, int idKat, int idPlatformy, int cel, int rokWydania, const QString &tworcy);
    bool aktualizujDaneMedium(int id, const QString &tytul, int idKat, int idPlatformy, int cel, int rokWydania, const QString &tworcy);
    bool usunMedium(int id);
    bool usunWieleMultimediow(const QList<int>& idList);
    bool zmienKategorieWielu(const QList<int>& idMultimediow, int nowaKategoriaId);
    bool ustawUlubione(int idMedium, bool ulubione);

    // --- Postęp (skrótowa ścieżka zapisu bez pełnego okna sesji) ---
    // aktualizujPostep celowo NIE emituje sygnału daneZmienione — patrz AppController.
    bool aktualizujPostep(int idMedium, const QString& status, int aktualna, int docelowa, int ocena);
    bool zacznijOdNowa(int idMedium);

    // --- Podejścia ---
    bool dodajPodejscie(int idMedium, const QString& status, int docelowa);
    bool aktualizujPodejscie(int idPodejscia, const QString& status, int aktualna, int docelowa, int ocena, const QString& recenzja);
    bool usunPodejscie(int idPodejscia);

    // --- Sesje (dziennik_aktywnosci) ---
    // Każda operacja na sesji jest transakcją, bo jednocześnie koryguje wartosc_aktualna w podejsciach.
    bool dodajSesje(int idPodejscia, int przyrost, int sekundy, const QString& notatka);
    bool aktualizujSesje(int idSesji, int przyrost, int sekundy, const QString& notatka);
    bool usunSesje(int idSesji);

    // --- Kategorie ---
    QMap<int, QString>           getCategories();
    QList<QPair<int, QString>>   pobierzKategorie();
    QList<QList<QVariant>>       pobierzSuroweKategorie();
    QMap<int, QString>           pobierzSlownikJednostek();
    QStringList                  pobierzUnikalneJednostki();
    int  dodajKategorie(const QString &nazwa, const QString &jednostka);
    bool aktualizujKategorie(int id, const QString &nazwa, const QString &jednostka);
    // usunPowiazane=true: kasuje multimedia tej kategorii; false: ustawia im id_kategorii = NULL.
    bool usunKategorie(int idKat, bool usunPowiazane);

    // --- Platformy ---
    QList<QPair<int, QString>> pobierzPlatformy();
    // Pełne dane platform wraz z typem nośnika: wiersz = [id, nazwa, typ_nosnika].
    QList<QList<QVariant>>     pobierzSurowePlatformy();
    int  dodajPlatforme(const QString &nazwa, const QString &typNosnika);
    bool aktualizujPlatforme(int id, const QString &nazwa, const QString &typNosnika);
    // Analogicznie jak usunKategorie — usunPowiazane kontroluje los multimedia.
    bool usunPlatforme(int idPlat, bool usunPowiazane);

    // --- Tagi ---
    QList<QPair<int, QString>>   pobierzTagi();
    QMap<int, QStringList>       pobierzPrzypisaniaTagow();
    int  dodajTag(const QString &nazwa);
    bool aktualizujTag(int id, const QString &nazwa);
    bool usunTag(int idTagu);
    // Pełna wymiana tagów: DELETE wszystkich przypisań + INSERT nowych (nie inkrementalne).
    bool ustawTagiDlaMedium(int idMedium, const QList<int>& idTagow);

    // --- Historia i podgląd ---
    QList<PodejscieHistoryczne> pobierzPelnaHistorie(int idMedium);
    // Uwaga: pole recenzja w zwróconych obiektach ma format "tytuł|||treść recenzji".
    QList<PodejscieHistoryczne> pobierzWszystkieRecenzje();

    // --- Statystyki ---
    QMap<QString, int>           getGlobalStats();
    QVariantMap                  pobierzStatystykiPodsumowania();
    QList<QVariantMap>           pobierzPodsumowanieKategorii();
    QList<QVariantMap>           pobierzPodsumowanieTagow();
    QVariantMap                  pobierzCiekawostkiStatystyczne();
    // Zwraca dane dla wykresu słupkowego; metryka: "czas" (sekundy), "sesje", "jednostki".
    QList<StatystykaAktywnosci>  pobierzSuroweDaneStatystyk(int zakresDni, const QString& metryka);

    // --- Inne ---
    QList<int> pobierzOstatnioAktywne(int limit);
};

#endif
