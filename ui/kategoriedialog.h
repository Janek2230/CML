#ifndef KATEGORIEDIALOG_H
#define KATEGORIEDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "appcontroller.h"

class KategorieDialog : public QDialog {
    Q_OBJECT

public:
    explicit KategorieDialog(AppController& controller, QWidget *parent = nullptr);
    ~KategorieDialog();

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
