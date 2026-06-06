#ifndef DETAILSWIDGET_H
#define DETAILSWIDGET_H

#include <QWidget>
#include <QTreeWidgetItem>
#include "appcontroller.h"

namespace Ui {
class DetailsWidget;
}

class DetailsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DetailsWidget(AppController& controller, QWidget *parent = nullptr);
    ~DetailsWidget();

    void ustawMedium(int idMedium);

signals:
    void daneZaktualizowane();

private slots:
    void obsluzZapisz();
    void obsluzNowePodejscie();
    void obsluzKlikniecieHistorii(QTreeWidgetItem *item);
    void obsluzEdytujZaznaczone();
    void obsluzUsunZaznaczone();
    void obsluzUlubione();
    void obsluzSzybkaSesja();
    void obsluzZakonczPodejscie();
    void obsluzCofnijZakonczenie();

private:
    enum class TypHistoriiElementu {
        Brak = 0,
        Podejscie = 1,
        Sesja = 2
    };

    void aktualizujStanPrzyciskowHistorii();
    void odswiezPrzyciskUlubione(bool czyUlubione);
    bool pokazDialogSesji(int& przyrost, int& sekundy, QString& notatka, const QString& tytul, bool edycja);

    Ui::DetailsWidget *ui;
    AppController& appController;
    int aktualneIdMedium = -1;
    void odswiezHistorie(int idMedium);
};

#endif
