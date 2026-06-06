#ifndef MULTIMEDIAFORMWIDGET_H
#define MULTIMEDIAFORMWIDGET_H

#include <QWidget>
#include "appcontroller.h"

namespace Ui {
class MultimediaFormWidget;
}

class MultimediaFormWidget : public QWidget
{
    // Q_OBJECT: makro wymagane w każdej klasie QObject, która korzysta z sygnałów/slotów,
    // właściwości (Q_PROPERTY) lub introspekcji.
    Q_OBJECT

public:
    explicit MultimediaFormWidget(AppController& controller, QWidget *parent = nullptr);
    ~MultimediaFormWidget();

    void przygotujFormularz(int idMedium = -1, int idDomyslnejKategorii = 0, int idDomyslnejPlatformy = 0);

signals:
    void formularzAnulowany();
    void daneZapisane();

private slots:
    void obsluzPotwierdzDodaj();
    void obsluzZmianeKategorii(int indeks);

    void obsluzSzybkaPlatforma();
    void obsluzSzybkaKategoria();
    void obsluzSzybkiTag();

private:
    Ui::MultimediaFormWidget *ui;
    AppController& appController;
    void uzupelnijTagi();

    bool czyTrybEdycji = false;
    int idEdytowanegoMedium = -1;

    void uzupelnijComboBoxy();

};

#endif
