#ifndef PLATFORMYDIALOG_H
#define PLATFORMYDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include "appcontroller.h"

class PlatformyDialog : public QDialog {
    Q_OBJECT

public:
    explicit PlatformyDialog(AppController& controller, QWidget *parent = nullptr);
    ~PlatformyDialog();

private slots:
    void onBtnDodajClicked();
    void onBtnEdytujClicked();
    void onBtnUsunClicked();

private:
    DatabaseManager& dbManager;
    QTableWidget *tabela;
    QPushButton *btnDodaj;
    QPushButton *btnEdytuj;
    QPushButton *btnUsun;
    QPushButton *btnZamknij;

    void wypelnijTabele();
};

#endif
