#include "appcontroller.h"
#include <QDebug>

// Po udanej operacji w bazie emitujemy daneZmienione(),

AppController::AppController(QObject *parent) : QObject(parent) {}

bool AppController::inicjalizujBaze() {
    if (!dbManager.openConnection()) {
        emit bladKrytyczny("Nie udało się połączyć z bazą danych. Sprawdź plik config.ini.");
        return false;
    }
    return true;
}

// Multimedia

QList<std::shared_ptr<Multimedia>> AppController::pobierzWszystkieMultimedia() {
    return dbManager.getAllMultimedia();
}

int AppController::dodajNoweMedium(const QString &tytul, int idKat, int idPlatformy, int cel, int rokWydania, const QString &tworcy) {
    int id = dbManager.dodajNoweMedium(tytul, idKat, idPlatformy, cel, rokWydania, tworcy);
    if (id > 0) {
        emit daneZmienione();
    }
    return id;
}

bool AppController::aktualizujDaneMedium(int id, const QString &tytul, int idKat, int idPlatformy, int cel, int rokWydania, const QString &tworcy) {
    if (dbManager.aktualizujDaneMedium(id, tytul, idKat, idPlatformy, cel, rokWydania, tworcy)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::usunMedium(int idMedium) {
    if (dbManager.usunMedium(idMedium)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::usunWieleMultimediow(const QList<int>& idList) {
    if (dbManager.usunWieleMultimediow(idList)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::zmienKategorieWielu(const QList<int>& idMultimediow, int nowaKategoriaId) {
    if (dbManager.zmienKategorieWielu(idMultimediow, nowaKategoriaId)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::ustawUlubione(int idMedium, bool ulubione) {
    if (dbManager.ustawUlubione(idMedium, ulubione)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

QList<int> AppController::pobierzOstatnioAktywne(int limit) {
    return dbManager.pobierzOstatnioAktywne(limit);
}

// Postęp

bool AppController::czyOsiagnietoCel(int aktualna, int docelowa) {
    return (aktualna >= docelowa && docelowa > 0);
}

bool AppController::aktualizujPostep(int idMedium, const QString& status, int aktualna, int docelowa, int ocena) {
    // Celowo nie emituje daneZmienione() — drzewo i dashboard odświeżają się tylko na żądanie.
    return dbManager.aktualizujPostep(idMedium, status, aktualna, docelowa, ocena);
}

bool AppController::zacznijOdNowa(int idMedium) {
    if (dbManager.zacznijOdNowa(idMedium)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

// Podejścia

bool AppController::dodajPodejscie(int idMedium, const QString& status, int docelowa) {
    if (dbManager.dodajPodejscie(idMedium, status, docelowa)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::aktualizujPodejscie(int idPodejscia, const QString& status, int aktualna, int docelowa, int ocena, const QString& recenzja) {
    if (dbManager.aktualizujPodejscie(idPodejscia, status, aktualna, docelowa, ocena, recenzja)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::usunPodejscie(int idPodejscia) {
    if (dbManager.usunPodejscie(idPodejscia)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

// Sesje

bool AppController::dodajSesje(int idPodejscia, int przyrost, int sekundy, const QString& notatka) {
    if (dbManager.dodajSesje(idPodejscia, przyrost, sekundy, notatka)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::aktualizujSesje(int idSesji, int przyrost, int sekundy, const QString& notatka) {
    if (dbManager.aktualizujSesje(idSesji, przyrost, sekundy, notatka)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::usunSesje(int idSesji) {
    if (dbManager.usunSesje(idSesji)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

// Kategorie

QMap<int, QString> AppController::getCategories() {
    return dbManager.getCategories();
}

QList<QPair<int, QString>> AppController::pobierzKategorie() {
    return dbManager.pobierzKategorie();
}

QList<AppController::KategoriaModel> AppController::pobierzPelneKategorie() {
    QList<KategoriaModel> lista;
    auto suroweDane = dbManager.pobierzSuroweKategorie();
    for (const auto& wiersz : suroweDane) {
        lista.append({wiersz[0].toInt(), wiersz[1].toString(), wiersz[2].toString()});
    }
    return lista;
}

QStringList AppController::pobierzUnikalneJednostki() {
    return dbManager.pobierzUnikalneJednostki();
}

QMap<int, QString> AppController::pobierzSlownikJednostek() {
    return dbManager.pobierzSlownikJednostek();
}

bool AppController::dodajKategorie(const QString &nazwa, const QString &jednostka) {
    if (dbManager.dodajKategorie(nazwa, jednostka) > 0) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::aktualizujKategorie(int id, const QString &nazwa, const QString &jednostka) {
    if (dbManager.aktualizujKategorie(id, nazwa, jednostka)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::usunKategorie(int idKat, bool usunPowiazane) {
    if (dbManager.usunKategorie(idKat, usunPowiazane)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

// Platformy

QList<QPair<int, QString>> AppController::pobierzPlatformy() {
    return dbManager.pobierzPlatformy();
}

QList<AppController::PlatformaModel> AppController::pobierzPelnePlatformy() {
    QList<PlatformaModel> lista;
    for (const auto& wiersz : dbManager.pobierzSurowePlatformy()) {
        lista.append({wiersz[0].toInt(), wiersz[1].toString(), wiersz[2].toString()});
    }
    return lista;
}

int AppController::dodajPlatforme(const QString &nazwa, const QString &typNosnika) {
    int id = dbManager.dodajPlatforme(nazwa, typNosnika);
    if (id > 0) {
        emit daneZmienione();
    }
    return id;
}

bool AppController::aktualizujPlatforme(int id, const QString &nazwa, const QString &typNosnika) {
    if (dbManager.aktualizujPlatforme(id, nazwa, typNosnika)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::usunPlatforme(int idPlat, bool usunPowiazane) {
    if (dbManager.usunPlatforme(idPlat, usunPowiazane)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

// Tagi

QList<QPair<int, QString>> AppController::pobierzTagi() {
    return dbManager.pobierzTagi();
}

QMap<int, QStringList> AppController::pobierzPrzypisaniaTagow() {
    return dbManager.pobierzPrzypisaniaTagow();
}

int AppController::dodajTag(const QString &nazwa) {
    int id = dbManager.dodajTag(nazwa);
    if (id > 0) {
        emit daneZmienione();
    }
    return id;
}

bool AppController::aktualizujTag(int id, const QString &nazwa) {
    if (dbManager.aktualizujTag(id, nazwa)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::usunTag(int idTagu) {
    if (dbManager.usunTag(idTagu)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::ustawTagiDlaMedium(int idMedium, const QList<int>& idTagow) {
    return dbManager.ustawTagiDlaMedium(idMedium, idTagow);
}

// Historia i podgląd

QList<PodejscieHistoryczne> AppController::pobierzHistorie(int idMedium) {
    return dbManager.pobierzPelnaHistorie(idMedium);
}

QList<PodejscieHistoryczne> AppController::pobierzWszystkieRecenzje() {
    return dbManager.pobierzWszystkieRecenzje();
}

// Statystyki

QMap<QString, int> AppController::getGlobalStats() {
    return dbManager.getGlobalStats();
}

QVariantMap AppController::pobierzStatystykiPodsumowania() {
    return dbManager.pobierzStatystykiPodsumowania();
}

QList<QVariantMap> AppController::pobierzPodsumowanieKategorii() {
    return dbManager.pobierzPodsumowanieKategorii();
}

QList<QVariantMap> AppController::pobierzPodsumowanieTagow() {
    return dbManager.pobierzPodsumowanieTagow();
}

QVariantMap AppController::pobierzCiekawostkiStatystyczne() {
    return dbManager.pobierzCiekawostkiStatystyczne();
}

QList<StatystykaAktywnosci> AppController::pobierzDaneDlaWykresu(int zakres, const QString& metryka) {
    auto dane = dbManager.pobierzSuroweDaneStatystyk(zakres, metryka);
    if (metryka == "czas") {
        for (auto& s : dane) s.wartosc /= 3600.0; // DB przechowuje sekundy; wykres używa godzin.
    }
    return dane;
}
