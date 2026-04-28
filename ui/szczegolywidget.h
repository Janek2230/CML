#ifndef SZCZEGOLYWIDGET_H
#define SZCZEGOLYWIDGET_H

#include <QWidget>
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
    void onBtnZacznijOdNowaClicked();

private:
    Ui::SzczegolyWidget *ui;
    AppController& appController;
    int aktualneIdMedium = -1;
    void odswiezHistorie(int idMedium);
};

#endif
