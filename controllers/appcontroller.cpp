#include "appcontroller.h"
#include <QDebug>

// Po udanej operacji w bazie emitujemy daneZmienione(),

AppController::AppController(QObject *parent) : QObject(parent) {}

bool AppController::inicjalizujBaze() {
    if (!menedzerBazy.otworzPolaczenie()) {
        return false;
    }
    return true;
}

// Multimedia

QList<std::shared_ptr<Multimedia>> AppController::pobierzWszystkieMultimedia() {
    return menedzerBazy.pobierzWszystkieMultimedia();
}

int AppController::dodajNoweMedium(const QString &tytul, int idKat, int idPlatformy, int cel, int rokWydania, const QString &tworcy) {
    int id = menedzerBazy.dodajNoweMedium(tytul, idKat, idPlatformy, cel, rokWydania, tworcy);
    if (id > 0) {
        emit daneZmienione();
    }
    return id;
}

bool AppController::aktualizujDaneMedium(int id, const QString &tytul, int idKat, int idPlatformy, int cel, int rokWydania, const QString &tworcy) {
    if (menedzerBazy.aktualizujDaneMedium(id, tytul, idKat, idPlatformy, cel, rokWydania, tworcy)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::usunMedium(int idMedium) {
    if (menedzerBazy.usunMedium(idMedium)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::usunWieleMultimediow(const QList<int>& listaId) {
    if (menedzerBazy.usunWieleMultimediow(listaId)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::zmienKategorieWielu(const QList<int>& idMultimediow, int nowaKategoriaId) {
    if (menedzerBazy.zmienKategorieWielu(idMultimediow, nowaKategoriaId)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::ustawUlubione(int idMedium, bool ulubione) {
    if (menedzerBazy.ustawUlubione(idMedium, ulubione)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

QList<int> AppController::pobierzOstatnioAktywne(int limit) {
    return menedzerBazy.pobierzOstatnioAktywne(limit);
}

// Postęp

bool AppController::czyOsiagnietoCel(int aktualna, int docelowa) {
    return (aktualna >= docelowa && docelowa > 0);
}

bool AppController::aktualizujPostep(int idMedium, const QString& status, int aktualna, int docelowa, int ocena) {
    // Celowo nie emituje daneZmienione() — drzewo i dashboard odświeżają się tylko na żądanie.
    return menedzerBazy.aktualizujPostep(idMedium, status, aktualna, docelowa, ocena);
}

// Podejścia

bool AppController::dodajPodejscie(int idMedium, const QString& status, int docelowa) {
    if (menedzerBazy.dodajPodejscie(idMedium, status, docelowa)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::aktualizujPodejscie(int idPodejscia, const QString& status, int aktualna, int docelowa, int ocena, const QString& recenzja) {
    if (menedzerBazy.aktualizujPodejscie(idPodejscia, status, aktualna, docelowa, ocena, recenzja)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::usunPodejscie(int idPodejscia) {
    if (menedzerBazy.usunPodejscie(idPodejscia)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

// Sesje

bool AppController::dodajSesje(int idPodejscia, int przyrost, int sekundy, const QString& notatka) {
    if (menedzerBazy.dodajSesje(idPodejscia, przyrost, sekundy, notatka)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::aktualizujSesje(int idSesji, int przyrost, int sekundy, const QString& notatka) {
    if (menedzerBazy.aktualizujSesje(idSesji, przyrost, sekundy, notatka)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::usunSesje(int idSesji) {
    if (menedzerBazy.usunSesje(idSesji)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

// Kategorie

QMap<int, QString> AppController::pobierzSlownikKategorii() {
    return menedzerBazy.pobierzSlownikKategorii();
}

QList<QPair<int, QString>> AppController::pobierzKategorie() {
    return menedzerBazy.pobierzKategorie();
}

QList<AppController::KategoriaModel> AppController::pobierzPelneKategorie() {
    QList<KategoriaModel> lista;
    auto suroweDane = menedzerBazy.pobierzSuroweKategorie();
    for (const auto& wiersz : suroweDane) {
        lista.append({wiersz[0].toInt(), wiersz[1].toString(), wiersz[2].toString()});
    }
    return lista;
}

QStringList AppController::pobierzUnikalneJednostki() {
    return menedzerBazy.pobierzUnikalneJednostki();
}

QMap<int, QString> AppController::pobierzSlownikJednostek() {
    return menedzerBazy.pobierzSlownikJednostek();
}

bool AppController::dodajKategorie(const QString &nazwa, const QString &jednostka) {
    if (menedzerBazy.dodajKategorie(nazwa, jednostka) > 0) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::aktualizujKategorie(int id, const QString &nazwa, const QString &jednostka) {
    if (menedzerBazy.aktualizujKategorie(id, nazwa, jednostka)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::usunKategorie(int idKat, bool usunPowiazane) {
    if (menedzerBazy.usunKategorie(idKat, usunPowiazane)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

// Platformy

QList<QPair<int, QString>> AppController::pobierzPlatformy() {
    return menedzerBazy.pobierzPlatformy();
}

QList<AppController::PlatformaModel> AppController::pobierzPelnePlatformy() {
    QList<PlatformaModel> lista;
    for (const auto& wiersz : menedzerBazy.pobierzSurowePlatformy()) {
        lista.append({wiersz[0].toInt(), wiersz[1].toString(), wiersz[2].toString()});
    }
    return lista;
}

int AppController::dodajPlatforme(const QString &nazwa, const QString &typNosnika) {
    int id = menedzerBazy.dodajPlatforme(nazwa, typNosnika);
    if (id > 0) {
        emit daneZmienione();
    }
    return id;
}

bool AppController::aktualizujPlatforme(int id, const QString &nazwa, const QString &typNosnika) {
    if (menedzerBazy.aktualizujPlatforme(id, nazwa, typNosnika)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::usunPlatforme(int idPlat, bool usunPowiazane) {
    if (menedzerBazy.usunPlatforme(idPlat, usunPowiazane)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

// Tagi

QList<QPair<int, QString>> AppController::pobierzTagi() {
    return menedzerBazy.pobierzTagi();
}

QMap<int, QStringList> AppController::pobierzPrzypisaniaTagow() {
    return menedzerBazy.pobierzPrzypisaniaTagow();
}

int AppController::dodajTag(const QString &nazwa) {
    int id = menedzerBazy.dodajTag(nazwa);
    if (id > 0) {
        emit daneZmienione();
    }
    return id;
}

bool AppController::aktualizujTag(int id, const QString &nazwa) {
    if (menedzerBazy.aktualizujTag(id, nazwa)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::usunTag(int idTagu) {
    if (menedzerBazy.usunTag(idTagu)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

bool AppController::ustawTagiDlaMedium(int idMedium, const QList<int>& idTagow) {
    return menedzerBazy.ustawTagiDlaMedium(idMedium, idTagow);
}

// Historia i podgląd

QList<HistoricalAttempt> AppController::pobierzHistorie(int idMedium) {
    return menedzerBazy.pobierzPelnaHistorie(idMedium);
}

QList<HistoricalAttempt> AppController::pobierzWszystkieRecenzje() {
    return menedzerBazy.pobierzWszystkieRecenzje();
}

// Statystyki

QMap<QString, int> AppController::pobierzStatystykiGlobalne() {
    return menedzerBazy.pobierzStatystykiGlobalne();
}

QVariantMap AppController::pobierzStatystykiPodsumowania() {
    return menedzerBazy.pobierzStatystykiPodsumowania();
}

QList<QVariantMap> AppController::pobierzPodsumowanieKategorii() {
    return menedzerBazy.pobierzPodsumowanieKategorii();
}

QList<QVariantMap> AppController::pobierzPodsumowanieTagow() {
    return menedzerBazy.pobierzPodsumowanieTagow();
}

QVariantMap AppController::pobierzCiekawostkiStatystyczne() {
    return menedzerBazy.pobierzCiekawostkiStatystyczne();
}

QList<ActivityStatistic> AppController::pobierzDaneDlaWykresu(int zakres, const QString& metryka) {
    auto dane = menedzerBazy.pobierzSuroweDaneStatystyk(zakres, metryka);
    if (metryka == "czas") {
        for (auto& s : dane) s.wartosc /= 3600.0; // DB przechowuje sekundy; wykres używa godzin.
    }
    return dane;
}
