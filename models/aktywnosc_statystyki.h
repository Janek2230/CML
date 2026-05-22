#ifndef AKTYWNOSC_STATYSTYKI_H
#define AKTYWNOSC_STATYSTYKI_H

#include <QString>

// DTO dla wykresu słupkowego aktywności.
// `data` to sformatowany string (np. "01.05", "05.2025") — format zależy od wybranego zakresu.
// `nazwaSerii` to etykieta serii (np. nazwa kategorii lub "Łącznie").
struct StatystykaAktywnosci {
    QString data;
    QString nazwaSerii;
    double wartosc;
};

#endif