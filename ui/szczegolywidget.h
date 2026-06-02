#ifndef SZCZEGOLYWIDGET_H
#define SZCZEGOLYWIDGET_H

#include <QWidget>
#include <QTreeWidgetItem>
#include "appcontroller.h"

namespace Ui {
class SzczegolyWidget;
}

class SzczegolyWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SzczegolyWidget(AppController& controller, QWidget *parent = nullptr);
    ~SzczegolyWidget();

    void ustawMedium(int idMedium);

signals:
    void daneZaktualizowane();

private slots:
    void onBtnZapiszClicked();
    void onBtnNowePodejscieClicked();
    void onHistoriaItemClicked(QTreeWidgetItem *item);
    void onBtnEdytujZaznaczoneClicked();
    void onBtnUsunZaznaczoneClicked();
    void onBtnUlubioneClicked();
    void onBtnSzybkaSesjaClicked();
    void onBtnZakonczPodejscieClicked();
    void onBtnCofnijZakonczenieClicked();

private:
    enum class TypHistoriiElementu {
        Brak = 0,
        Podejscie = 1,
        Sesja = 2
    };

    void aktualizujStanPrzyciskowHistorii();
    void odswiezPrzyciskUlubione(bool czyUlubione);
    bool pokazDialogSesji(int& przyrost, int& sekundy, QString& notatka, const QString& tytul, bool edycja);

    Ui::SzczegolyWidget *ui;
    AppController& appController;
    int aktualneIdMedium = -1;
    void odswiezHistorie(int idMedium);
};

#endif
