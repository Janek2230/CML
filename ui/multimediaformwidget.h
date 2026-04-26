#ifndef MULTIMEDIAFORMWIDGET_H
#define MULTIMEDIAFORMWIDGET_H

#include <QWidget>
#include "databasemanager.h"

namespace Ui {
class MultimediaFormWidget;
}

class MultimediaFormWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MultimediaFormWidget(DatabaseManager& db, QWidget *parent = nullptr);
    ~MultimediaFormWidget();

    void przygotujFormularz(int idMedium = -1, int idDomyslnejKategorii = 0, int idDomyslnejPlatformy = 0);

signals:
    void formularzAnulowany();
    void daneZapisane();

private slots:
    void onBtnPotwierdzDodajClicked();
    void onComboNowaKategoriaChanged(int index);

    void onBtnSzybkaPlatformaClicked();
    void onBtnSzybkaKategoriaClicked();

private:
    Ui::MultimediaFormWidget *ui;
    DatabaseManager& dbManager;

    bool czyTrybEdycji = false;
    int idEdytowanegoMedium = -1;

    void uzupelnijComboBoxy();

};

#endif