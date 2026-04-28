#include "appcontroller.h"
#include <QDebug>

AppController::AppController(QObject *parent) : QObject(parent) {}

bool AppController::inicjalizujBaze() {
    if (!dbManager.openConnection()) {
        emit bladKrytyczny("Nie udało się połączyć z bazą danych. Sprawdź plik config.ini.");
        return false;
    }
    return true;
}

QList<QVariantMap> AppController::pobierzDaneDlaWykresu(int zakres, const QString& metryka) {
    auto suroweDane = dbManager.pobierzSuroweDaneStatystyk(zakres, metryka);
    QList<QVariantMap> sformatowaneDane;

    for (const auto& s : suroweDane) {
        QVariantMap mapa;
        mapa["data"] = s.data;
        mapa["seria"] = s.nazwaSerii;

        if (metryka == "czas") {
            mapa["wartosc"] = s.wartosc / 3600.0;
        } else {
            mapa["wartosc"] = s.wartosc;
        }

        sformatowaneDane.append(mapa);
    }

    return sformatowaneDane;
}

QList<std::shared_ptr<Multimedia>> AppController::pobierzWszystkieMultimedia() {
    return dbManager.getAllMultimedia();
}

QMap<QString, int> AppController::getGlobalStats() {
    return dbManager.getGlobalStats();
}

bool AppController::czyOsiagnietoCel(int aktualna, int docelowa) {
    return (aktualna >= docelowa && docelowa > 0);
}

bool AppController::aktualizujPostep(int idMedium, const QString& status, int aktualna, int docelowa, int ocena) {
    return dbManager.aktualizujPostep(idMedium, status, aktualna, docelowa, ocena);
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

bool AppController::usunPlatforme(int idPlat, bool usunPowiazane) {
    if (dbManager.usunPlatforme(idPlat, usunPowiazane)) {
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

bool AppController::usunWieleMultimediow(const QList<int>& idList) {
    if (dbManager.usunWieleMultimediow(idList)) {
        emit daneZmienione();
        return true;
    }
    return false;
}

QList<AppController::KategoriaModel> AppController::pobierzPelneKategorie() {
    QList<KategoriaModel> lista;
    auto suroweDane = dbManager.pobierzSuroweKategorie();
    for(const auto& wiersz : suroweDane) {
        lista.append({wiersz[0].toInt(), wiersz[1].toString(), wiersz[2].toString()});
    }
    return lista;
}

QStringList AppController::pobierzDostepneStatusy() {
    return {"Planowane", "W trakcie", "Wstrzymane", "Ukończone", "Porzucone"};
}

QMap<int, QString> AppController::getCategories() {
    return dbManager.getCategories();
}

QList<std::shared_ptr<Multimedia>> AppController::pobierzKupkeWstydu() {
    auto wszystko = pobierzWszystkieMultimedia();
    QList<std::shared_ptr<Multimedia>> pobraneDane;
    QDateTime teraz = QDateTime::currentDateTime();

    for (const auto& m : wszystko) {
        if (m->getStatus() == "Planowane") {
            if (m->getDataDodania().daysTo(teraz) > 180) {
                pobraneDane.append(m);
            }
        } else if (m->getStatus() == "W trakcie") {
            if (m->getDataOstatniejAktywnosci().isValid() && m->getDataOstatniejAktywnosci().daysTo(teraz) > 30) {
                pobraneDane.append(m);
            }
        }
    }
    return pobraneDane;
}

bool AppController::zacznijOdNowa(int idMedium) {
    if(dbManager.zacznijOdNowa(idMedium)) { emit daneZmienione(); return true; }
    return false;
}
bool AppController::dodajNoweMedium(const QString &tytul, int idKat, int idPlatformy, int cel) {
    if(dbManager.dodajNoweMedium(tytul, idKat, idPlatformy, cel)) { emit daneZmienione(); return true; }
    return false;
}
bool AppController::aktualizujDaneMedium(int id, const QString &tytul, int idKat, int idPlatformy, int cel) {
    if(dbManager.aktualizujDaneMedium(id, tytul, idKat, idPlatformy, cel)) { emit daneZmienione(); return true; }
    return false;
}
int AppController::dodajPlatforme(const QString &nazwa) {
    int id = dbManager.dodajPlatforme(nazwa);
    if(id > 0) { emit daneZmienione(); }
    return id;
}
bool AppController::aktualizujPlatforme(int id, const QString &nazwa) {
    if(dbManager.aktualizujPlatforme(id, nazwa)) { emit daneZmienione(); return true; }
    return false;
}
QList<int> AppController::pobierzOstatnioAktywne(int limit) {
    return dbManager.pobierzOstatnioAktywne(limit);
}

QStringList AppController::pobierzUnikalneJednostki() { return dbManager.pobierzUnikalneJednostki(); }
QList<QPair<int, QString>> AppController::pobierzKategorie() { return dbManager.pobierzKategorie(); }
QList<QPair<int, QString>> AppController::pobierzPlatformy() { return dbManager.pobierzPlatformy(); }
QMap<int, QString> AppController::pobierzSlownikJednostek() { return dbManager.pobierzSlownikJednostek(); }
