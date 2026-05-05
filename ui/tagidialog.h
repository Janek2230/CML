#ifndef TAGIDIALOG_H
#define TAGIDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include "appcontroller.h"

class TagiDialog : public QDialog {
    Q_OBJECT

public:
    explicit TagiDialog(AppController& controller, QWidget *parent = nullptr);
    ~TagiDialog();

private slots:
    void onBtnDodajClicked();
    void onBtnEdytujClicked();
    void onBtnUsunClicked();

private:
    AppController& appController;
    QTableWidget *tabela;
    QPushButton *btnDodaj;
    QPushButton *btnEdytuj;
    QPushButton *btnUsun;
    QPushButton *btnZamknij;

    void wypelnijTabele();
};

#endif
