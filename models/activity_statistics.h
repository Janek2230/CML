#ifndef ACTIVITY_STATISTICS_H
#define ACTIVITY_STATISTICS_H

#include <QString>

// DTO dla wykresu słupkowego aktywności.
// `data` to sformatowany string (np. "01.05", "05.2025") — format zależy od wybranego zakresu.
// `nazwaSerii` to etykieta serii (np. nazwa kategorii lub "Łącznie").
// `wartosc` to wartość metryki dla słupka (sekundy lub liczba sztuk); dla metryki czasu
// jest dzielona przez 3600 (na godziny) w AppController::pobierzDaneDlaWykresu().
struct ActivityStatistic {
    QString data;
    QString nazwaSerii;
    double wartosc = 0.0;
};

#endif