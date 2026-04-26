#ifndef MULTIMEDIAFORMWIDGET_H
#define MULTIMEDIAFORMWIDGET_H

#include <QWidget>
#include "databasemanager.h"
#include "multimediaformwidget.h"

namespace Ui {
class MultimediaFormWidget;
}

class MultimediaFormWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MultimediaFormWidget(DatabaseManager& db, QWidget *parent = nullptr);
    ~MultimediaFormWidget();

    // Metoda zastępująca Twoje stare przygotujFormularz z MainWindow
    void przygotujFormularz(int idMedium = -1, int idDomyslnejKategorii = 0, int idDomyslnejPlatformy = 0);

signals:
    // SYGNAŁY - formularz krzyczy, że coś się stało
    void formularzAnulowany();
    void daneZapisane(); // MainWindow tego posłucha, żeby odświeżyć drzewo

private slots:
    // Slot podłączony pod przycisk "Zapisz/Dodaj"
    void onBtnPotwierdzDodajClicked();

    // Slot podłączony pod zmianę kategorii, żeby zmienić jednostkę w etykiecie
    void onComboNowaKategoriaChanged(int index);

    void onBtnSzybkaPlatformaClicked();
    void onBtnSzybkaKategoriaClicked();

private:
    Ui::MultimediaFormWidget *ui;
    DatabaseManager& dbManager;

    MultimediaFormWidget *formularzWidget;

    bool czyTrybEdycji = false;
    int idEdytowanegoMedium = -1;

    void uzupelnijComboBoxy();

};

#endif // MULTIMEDIAFORMWIDGET_H