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
