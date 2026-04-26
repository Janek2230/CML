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

    // Tą metodą MainWindow będzie pompować dane do widoku
    void ustawMedium(int idMedium);

signals:
    // Sygnał emitowany, gdy zapiszemy postęp albo wyzerujemy grę
    void daneZaktualizowane();

private slots:
    void onBtnZapiszClicked();
    void onBtnZacznijOdNowaClicked();

private:
    Ui::SzczegolyWidget *ui;
    DatabaseManager& dbManager;
    int aktualneIdMedium = -1; // Zapamiętujemy, co obecnie wyświetlamy
};

#endif // SZCZEGOLYWIDGET_H