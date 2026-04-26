#ifndef SZCZEGOLYWIDGET_H
#define SZCZEGOLYWIDGET_H

#include <QWidget>
#include "databasemanager.h"

namespace Ui {
class SzczegolyWidget;
}

class SzczegolyWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SzczegolyWidget(DatabaseManager& db, QWidget *parent = nullptr);
    ~SzczegolyWidget();

    void ustawMedium(int idMedium);

signals:
    void daneZaktualizowane();

private slots:
    void onBtnZapiszClicked();
    void onBtnZacznijOdNowaClicked();

private:
    Ui::SzczegolyWidget *ui;
    DatabaseManager& dbManager;
    int aktualneIdMedium = -1;
};

#endif